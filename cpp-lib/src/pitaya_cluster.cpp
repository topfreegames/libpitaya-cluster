#include "pitaya.h"
#include "pitaya/c_wrapper.h"
#include "pitaya/cluster.h"
#include "pitaya/etcdv3_service_discovery.h"
#include "pitaya/etcdv3_service_discovery/config.h"
#include "pitaya/grpc/rpc_client.h"
#include "pitaya/grpc/rpc_server.h"
#include "pitaya/nats/rpc_client.h"
#include "pitaya/nats/rpc_server.h"
#include "pitaya/protos/msg.pb.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/service_discovery.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <thread>

using namespace std;
using namespace pitaya;
using namespace pitaya::nats;
using pitaya::etcdv3_service_discovery::Etcdv3ServiceDiscovery;
using pitaya::service_discovery::ServiceDiscovery;

int x;
std::atomic<long> qps(0);

static void
SignalHandler(int signo)
{
    Cluster::Instance().Terminate();
    exit(0);
}

protos::Response
RpcHandler(const protos::Request& req)
{
    cout << "rpc handler called with route: " << req.msg().route() << endl;
    if (req.has_msg()) {
        cout << "   data = " << req.msg().data() << endl;
    }
    auto res = protos::Response();
    res.set_data("RPC went ok!");
    return res;
}

void
print()
{
    while (true) {
        std::cout << "qps: " << qps << std::endl;
        qps = 0;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void
loopSendRpc(shared_ptr<spdlog::logger> logger, int tid)
{
    auto msg = new protos::Msg();
    auto session = new protos::Session();
    session->set_id(1);
    session->set_uid("uid123");

    msg->set_route("csharp.testHandler.entry");

    protos::Request req;
    req.set_allocated_session(session);

    req.set_type(protos::RPCType::Sys);
    req.set_allocated_msg(msg);
    req.set_frontendid("testfid");
    protos::Response res;
    while (true) {
        auto err = Cluster::Instance().RPC("csharp.testHandler.entry", req, res);
        if (err) {
            cout << "received error:" << err.value().msg << endl;
        } else {
            // cout << "received answer: " << res.data() << endl;
        }
        qps++;
    }
}

int
main()
{
#if 1
    signal(SIGTERM, SignalHandler);
    signal(SIGINT, SignalHandler);

    auto logger = spdlog::stdout_color_mt("main");
    logger->set_level(spdlog::level::debug);

    pitaya::Server server(Server::Kind::Frontend, "someid", "sometype");
    NatsConfig natsConfig("nats://localhost:4222", 1000, 3000, 3, 100);

    etcdv3_service_discovery::Config sdConfig;
    sdConfig.endpoints = "http://127.0.0.1:2379";
    sdConfig.etcdPrefix = "pitaya/";
    sdConfig.logHeartbeat = false;
    sdConfig.logServerSync = true;
    sdConfig.logServerDetails = true;
    sdConfig.heartbeatTTLSec = std::chrono::seconds(20);
    sdConfig.syncServersIntervalSec = std::chrono::seconds(20);

    GrpcConfig grpcConfig;
    grpcConfig.host = "127.0.0.1";
    grpcConfig.port = 5440;
    grpcConfig.connectionTimeout = std::chrono::seconds(2);

    try {
#if 1
        Cluster::Instance().InitializeWithGrpc(
            std::move(grpcConfig), std::move(sdConfig), server, RpcHandler, "main");
#else
        Cluster::Instance().InitializeWithNats(
            std::move(natsConfig), std::move(sdConfig), server, RpcHandler, "main");
#endif

        {
            // INIT
            thread thr(print);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // FINISH
            thread threads[1];
            for (int i = 0; i < 1; i++) {
                threads[i] = thread(loopSendRpc, logger, i);
            }
            thr.join();
        }

        Cluster::Instance().Terminate();
    } catch (const PitayaException& e) {
        cout << e.what() << endl;
        cin >> x;
    }
#endif
}
