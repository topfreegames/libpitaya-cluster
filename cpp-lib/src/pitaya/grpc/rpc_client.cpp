#include "pitaya/grpc/rpc_client.h"

#include "pitaya/constants.h"
#include "pitaya/utils.h"
#include "pitaya/utils/grpc.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include <assert.h>
#include <cpprest/json.h>
#include <grpcpp/create_channel.h>

namespace json = web::json;
using boost::optional;

namespace pitaya {

static constexpr const char* kLogTag = "grpc_client";

GrpcClient::GrpcClient(GrpcConfig config,
                       std::shared_ptr<service_discovery::ServiceDiscovery> serviceDiscovery,
                       std::unique_ptr<BindingStorage> bindingStorage,
                       const char* loggerName)
    : GrpcClient(config,
                 std::move(serviceDiscovery),
                 std::move(bindingStorage),
                 [](std::shared_ptr<grpc::ChannelInterface> channel)
                     -> std::unique_ptr<protos::Pitaya::StubInterface> {
                     return protos::Pitaya::NewStub(std::move(channel));
                 },
                 loggerName)
{}

GrpcClient::GrpcClient(GrpcConfig config,
                       std::shared_ptr<service_discovery::ServiceDiscovery> serviceDiscovery,
                       std::unique_ptr<BindingStorage> bindingStorage,
                       CreateStubFunc createStub,
                       const char* loggerName)
    : _log(utils::CloneLoggerOrCreate(loggerName, kLogTag))
    , _config(std::move(config))
    , _stubsForServers()
    , _serviceDiscovery(std::move(serviceDiscovery))
    , _createStub(std::move(createStub))
    , _bindingStorage(std::move(bindingStorage))
{
    assert(_bindingStorage != nullptr);
    assert(_serviceDiscovery != nullptr);

    _log->info("Registering gRPC client as a listener to the service discovery");

    // FIXME: this call here makes the service discovery call the GRPC client sometimes when the
    // object is NOT initialized. This is currently not causing problems, but should be changed in
    // order to avoid future issues.
    _serviceDiscovery->AddListener(this);
    _log->info("gRPC RPC client created");
}

GrpcClient::~GrpcClient()
{
    _log->info("Unregistering gRPC client as a listener to the service discovery");
    _serviceDiscovery->RemoveListener(this);
}

static protos::Response
NewErrorResponse(const std::string& errorCode, const std::string& msg)
{
    auto error = new protos::Error();
    error->set_code(errorCode);
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
            return NewErrorResponse(constants::kCodeInternalError, msg);
        } else {
            _log->debug("Found server on the connections map");
        }
    }

    // The stub is contained in the map. We then retrieve it and call the desired RPC function.
    {
        std::lock_guard<decltype(_stubsForServers)> lock(_stubsForServers);
        protos::Pitaya::StubInterface* stub = _stubsForServers[target.Id()].get();

        protos::Response res;
        grpc::ClientContext context;
        _log->debug("Making RPC call with {} milliseconds of timeout", _config.clientRpcTimeout.count());
        context.set_deadline(std::chrono::system_clock::now() + _config.clientRpcTimeout);
        auto status = stub->Call(&context, req, &res);

        if (!status.ok()) {
            auto msg = fmt::format("Call RPC failed: {}", status.error_message());
            if (!status.error_details().empty()) {
                msg += ", details: " + status.error_details();
            }
            _log->error(msg);
            if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED) {
                return NewErrorResponse(constants::kCodeTimeout, msg);
            } else {
                return NewErrorResponse(constants::kCodeInternalError, msg);
            }
        }

        return res;
    }
}

optional<PitayaError>
GrpcClient::SendPushToUser(const std::string& providedServerId,
                           const std::string& serverType,
                           const protos::Push& push)
{
    std::string serverId;

    // If the user provided an empty server id, we need to fetch it from the
    // binding storage, otherwise we use the one provided.
    if (providedServerId.empty()) {
        try {
            serverId = _bindingStorage->GetUserFrontendId(push.uid(), serverType);
        } catch (const PitayaException& exc) {
            return PitayaError(constants::kCodeInternalError, exc.what());
        }
    } else {
        serverId = providedServerId;
    }

    // TODO: It it good to have two locks here instead of one??
    {
        std::lock_guard<decltype(_stubsForServers)> lock(_stubsForServers);
        if (_stubsForServers.Find(serverId) == _stubsForServers.end()) {
            auto msg = fmt::format(
                "Cannot push to server {}, since it is not added to the connections map", serverId);
            _log->error(msg);
            return PitayaError(constants::kCodeInternalError, msg);
        }
    }

    std::lock_guard<decltype(_stubsForServers)> lock(_stubsForServers);
    protos::Pitaya::StubInterface* stub = _stubsForServers[serverId].get();

    protos::Response res;
    grpc::ClientContext context;
    auto status = stub->PushToUser(&context, push, &res);

    if (!status.ok()) {
        auto msg = fmt::format("Push failed: {}", status.error_message());
        return PitayaError(constants::kCodeInternalError, msg);
    }

    return boost::none;
}

optional<PitayaError>
GrpcClient::SendKickToUser(const std::string& providedServerId,
                           const std::string& serverType,
                           const protos::KickMsg& kick)
{
    protos::KickAnswer kickAns;
    std::string serverId;

    // If the user provided an empty server id, we need to fetch it from the
    // binding storage, otherwise we use the one provided.
    if (providedServerId.empty()) {
        try {
            serverId = _bindingStorage->GetUserFrontendId(kick.userid(), serverType);
        } catch (const PitayaException& exc) {
            return PitayaError(constants::kCodeInternalError,
                               fmt::format("Cannot kick user {} on server {}, since it was not "
                                           "found on the binding storage",
                                           kick.userid(),
                                           serverType));
        }
    } else {
        serverId = providedServerId;
    }

    {
        std::lock_guard<decltype(_stubsForServers)> lock(_stubsForServers);
        if (_stubsForServers.Find(serverId) == _stubsForServers.end()) {
            return PitayaError(
                constants::kCodeInternalError,
                fmt::format(
                    "Cannot kick on server {}, since it is not added to the connections map",
                    serverId));
        }
    }

    std::lock_guard<decltype(_stubsForServers)> lock(_stubsForServers);
    protos::Pitaya::StubInterface* stub = _stubsForServers[serverId].get();

    grpc::ClientContext context;
    auto status = stub->KickUser(&context, kick, &kickAns);

    if (!status.ok()) {
        return PitayaError(constants::kCodeInternalError,
                           fmt::format("Kick failed: {}", status.error_message()));
    }

    return boost::none;
}

// ==================================================
// service_discovery::Listener implementation
// ==================================================

void
GrpcClient::ServerAdded(const pitaya::Server& server)
{
    if (server.Metadata() == "") {
        // Ignore the server, since it has no metadata.
        return;
    }

    // First, we need to get the server host (address and port)
    std::string address;
    try {
        address = utils::GetGrpcAddressFromServer(server);
    } catch (const PitayaException& exc) {
        // We were not able to fetch the address from the server,
        // therefore we just ignoore it.
        return;
    }

    auto channel = grpc::CreateChannel(address, ::grpc::InsecureChannelCredentials());
    auto stub = _createStub(std::move(channel));

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

} // namespace pitaya
