#include "pitaya.h"
#include "pitaya/cluster.h"
#include "pitaya/etcdv3_service_discovery.h"
#include "pitaya/etcdv3_service_discovery/config.h"
#include "pitaya/grpc/rpc_client.h"
#include "pitaya/grpc/rpc_server.h"
#include "pitaya/nats/rpc_client.h"
#include "pitaya/nats/rpc_server.h"
#include "pitaya/service_discovery.h"
#include "protos/msg.pb.h"
#include "protos/request.pb.h"

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
    auto res = protos::Response();
    res.set_data("RPC went ok!");
    return res;
}

int
main()
{
    signal(SIGTERM, SignalHandler);
    signal(SIGINT, SignalHandler);

    auto logger = spdlog::stdout_color_mt("main");
    logger->set_level(spdlog::level::debug);

    pitaya::Server server("someid",
                          "sometype",
                          {
                              { "grpc-host", "127.0.0.1" },
                              { "grpc-port", "58000" },
                          });
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
    grpcConfig.host = "http://127.0.0.1";
    grpcConfig.port = 5440;
    grpcConfig.connectionTimeout = std::chrono::seconds(2);

    try {
        Cluster::Instance().InitializeWithGrpc(
            std::move(grpcConfig), std::move(sdConfig), server, RpcHandler, "main");

        {
            // INIT
            auto msg = new protos::Msg();
            //            msg->set_data("helloww");
            msg->set_route("room.room.testremote");

            protos::Request req;
            req.set_allocated_msg(msg);
            std::vector<uint8_t> buffer(req.ByteSizeLong());
            req.SerializeToArray(buffer.data(), buffer.size());

            // FINISH
            protos::Response res;
            auto err = Cluster::Instance().RPC("room.room.testremote", req, res);
            if (err) {
                cout << "received error:" << err.value().msg << endl;
            } else {
                cout << "received answer: " << res.data() << endl;
            }
        }

        cin >> x;

        Cluster::Instance().Terminate();
    } catch (const PitayaException& e) {
        cout << e.what() << endl;
        cin >> x;
    }
}
