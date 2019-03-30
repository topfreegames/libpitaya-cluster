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

void
RpcHandler(const protos::Request& req, pitaya::Rpc* rpc)
{
    std::cout << "rpc handler called with route: " << req.msg().route() << std::endl;
    if (req.has_msg()) {
        std::cout << "   data = " << req.msg().data() << std::endl;
    }
    auto res = protos::Response();
    res.set_data("RPC went ok!");

    rpc->Finish(res);
}

void
Print()
{
    while (true) {
        std::cout << "qps: " << qps << std::endl;
        qps = 0;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void
LoopSendRpc(std::shared_ptr<spdlog::logger> logger, int tid)
{
    auto msg = new protos::Msg();
    auto session = new protos::Session();
    session->set_id(1);
    session->set_uid("uid123");

    msg->set_route("sometype.testHandler.entry");

    protos::Request req;
    req.set_allocated_session(session);

    req.set_type(protos::RPCType::Sys);
    req.set_allocated_msg(msg);
    req.set_frontendid("testfid");
    protos::Response res;
    while (true) {
        auto err = Cluster::Instance().RPC("sometype.testHandler.entry", req, res);
        if (err) {
            std::cout << "received error:" << err.value().msg << std::endl;
        } else {
            std::cout << "received answer: " << res.data() << std::endl;
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

    etcdv3_service_discovery::Config sdConfig;
    sdConfig.endpoints = "http://127.0.0.1:2379";
    sdConfig.etcdPrefix = "pitaya/";
    sdConfig.logHeartbeat = false;
    sdConfig.logServerSync = true;
    sdConfig.logServerDetails = true;
    sdConfig.heartbeatTTLSec = std::chrono::seconds(20);
    sdConfig.syncServersIntervalSec = std::chrono::seconds(20);

    EtcdBindingStorageConfig bindingStorageConfig;
    bindingStorageConfig.endpoint = sdConfig.endpoints;
    bindingStorageConfig.etcdPrefix = "pitaya/";
    bindingStorageConfig.leaseTtl = std::chrono::seconds(5);

    try {
#if 1
        GrpcConfig grpcConfig;
        grpcConfig.host = "127.0.0.1";
        grpcConfig.port = 5440;
        grpcConfig.connectionTimeout = std::chrono::seconds(2);

        Cluster::Instance().InitializeWithGrpc(std::move(grpcConfig),
                                               std::move(sdConfig),
                                               std::move(bindingStorageConfig),
                                               server,
                                               RpcHandler,
                                               "main");
#else
        NatsConfig natsConfig("nats://localhost:4222", 1000, 3000, 3, 100);

        Cluster::Instance().InitializeWithNats(
            std::move(natsConfig), std::move(sdConfig), server, RpcHandler, "main");
#endif
        {
            // // INIT
            std::thread thr(Print);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // FINISH
            std::thread threads[1];
            for (int i = 0; i < 1; i++) {
                threads[i] = std::thread(LoopSendRpc, logger, i);
            }

            // Now, wait for RPCs
            for (;;) {
                auto rpcData = Cluster::Instance().WaitForRpc();

                logger->debug("Processing new rpc...");

                protos::Response res;
                res.set_data("MY DATA MAAAAAAAAAAAN");

                rpcData.rpc->Finish(res);
            }

            thr.join();
        }

        std::cin >> x;

        Cluster::Instance().Terminate();
    } catch (const PitayaException& e) {
        std::cout << e.what() << std::endl;
        std::cin >> x;
    }
#endif
}
