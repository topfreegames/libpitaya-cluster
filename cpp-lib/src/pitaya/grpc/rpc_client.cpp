#include "pitaya/grpc/rpc_client.h"

#include "pitaya/constants.h"
#include "pitaya/utils/grpc.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include <assert.h>
#include <cpprest/json.h>
#include <grpcpp/create_channel.h>

namespace json = web::json;
using grpc::Channel;

namespace pitaya {

GrpcClient::GrpcClient(GrpcConfig config,
                       std::shared_ptr<service_discovery::ServiceDiscovery> serviceDiscovery,
                       const char* loggerName)
    : _log(loggerName ? spdlog::get(loggerName)->clone("grpc_client")
                      : spdlog::stdout_color_mt("grpc_client"))
    , _config(std::move(config))
    , _serviceDiscovery(std::move(serviceDiscovery))
{
    assert(_serviceDiscovery != nullptr);
    _log->info("Registering gRPC client as a listener to the service discovery");
    _serviceDiscovery->AddListener(this);
    _log->info("gRPC RPC client created");
}

GrpcClient::~GrpcClient()
{
    _log->info("Unregistering gRPC client as a listener to the service discovery");
    _serviceDiscovery->RemoveListener(this);
}

static protos::Response
NewErrorResponse(const std::string& msg)
{
    auto error = new protos::Error();
    error->set_code(constants::kCodeInternalError);
    error->set_msg(msg);

    protos::Response res;
    res.set_allocated_error(error);
    return res;
}

protos::Response
GrpcClient::Call(const pitaya::Server& target, const protos::Request& req)
{
    // In order to send an rpc to a server, we need to first find the connection to the
    // server in the map.

    {
        std::lock_guard<decltype(_stubsForServers)> lock(_stubsForServers);
        if (_stubsForServers.Find(target.Id()) == _stubsForServers.end()) {
            auto msg = fmt::format(
                "Cannot call server {}, since it is not added to the connections map", target.Id());
            _log->error(msg);
            return NewErrorResponse(msg);
        }
    }

    // The stub is contained in the map. We then retrieve it and call the desired RPC function.
    {
        std::lock_guard<decltype(_stubsForServers)> lock(_stubsForServers);
        protos::Pitaya::Stub* stub = _stubsForServers[target.Id()].get();

        protos::Response res;
        ::grpc::ClientContext context;
        auto status = stub->Call(&context, req, &res);

        if (!status.ok()) {
            auto msg = fmt::format("Call RPC failed: {}", status.error_message());
            _log->error(msg);
            return NewErrorResponse(msg);
        }

        return res;
    }
}

// ==================================================
// service_discovery::Listener implementation
// ==================================================

void
GrpcClient::ServerAdded(const pitaya::Server& server)
{
    if (server.Metadata() == "") {
        _log->debug("Ignoring server {}, since it does not support gRPC", server.Id());
        return;
    }

    // First, we need to get the server host (address and port)

    std::string address;
    try {
        address = utils::GetGrpcAddressFromServer(server);
    } catch (const PitayaException& exc) {
        _log->warn("Cannot fetch gRPC info from server {}: {}", server.Id(), exc.what());
        return;
    }

    auto channel = ::grpc::CreateChannel(address, ::grpc::InsecureChannelCredentials());

    if (channel->WaitForConnected(std::chrono::system_clock::now() + _config.connectionTimeout)) {
        _log->info("Successfully connected to gPRC server at {}", address);
    } else {
        _log->warn("Failed to connect to gRPC server at {}", address);
    }

    auto stub = protos::Pitaya::NewStub(std::move(channel));

    std::lock_guard<decltype(_stubsForServers)> lock(_stubsForServers);
    _stubsForServers[server.Id()] = std::move(stub);
    _log->debug("New server {} added", server.Id());
}

void
GrpcClient::ServerRemoved(const pitaya::Server& server)
{
    std::lock_guard<decltype(_stubsForServers)> lock(_stubsForServers);

    if (_stubsForServers.Find(server.Id()) == _stubsForServers.end()) {
        _log->warn("Server {} was removed, however it was not synchronized in the grpc rpc client",
                   server.Id());
        return;
    }

    _stubsForServers.Erase(server.Id());
    _log->debug("Removed server {}", server.Id());
}

protos::Response
GrpcClient::SendPushToUser(const std::string& server_id, const std::string& server_type, const protos::Push& push) {
    if (server_id.empty()){
      //TODO implement this with binding storage
    } 
    std::lock_guard<decltype(_stubsForServers)> lock(_stubsForServers);
    protos::Pitaya::Stub* stub = _stubsForServers[server_id].get();

    protos::Response res;
    ::grpc::ClientContext context;
    auto status = stub->PushToUser(&context, push, &res);

    if (!status.ok()) {
        auto msg = fmt::format("Push failed: {}", status.error_message());
        return NewErrorResponse(msg);
    }

    return res;

}

} // namespace pitaya
