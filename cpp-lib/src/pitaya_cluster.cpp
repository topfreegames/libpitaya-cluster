#include "pitaya.h"
#include "pitaya/cluster.h"
#include "pitaya/etcdv3_service_discovery/config.h"
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
using pitaya::service_discovery::ServiceDiscovery;

int x;

protos::Response
rpc_handler(protos::Request req)
{
    cout << "rpc handler called with route: " << req.msg().route() << endl;
    auto res = protos::Response();
    auto res2 = protos::Response();
    res2.set_data("ok!");
    std::string serialized;
    res2.SerializeToString(&serialized);
    res.set_data(serialized);
    return res;
}

int
main()
{
    auto logger = spdlog::stdout_color_mt("main");
    logger->set_level(spdlog::level::debug);
    //    auto logger = spdlog::basic_logger_mt("main", "custom-log.log");

    Server server("someid", "sometype", "{\"user\": \"Carro\"}");
    NatsConfig nats_config("nats://localhost:4222", 1000, 3000, 3, 100);

    etcdv3_service_discovery::Config sdConfig;
    sdConfig.endpoints = "http://127.0.0.1:4001";
    sdConfig.etcdPrefix = "pitaya/";
    sdConfig.logHeartbeat = false;
    sdConfig.logServerSync = false;

    try {
        Cluster::Instance().Initialize(
            std::move(sdConfig), std::move(nats_config), std::move(server), rpc_handler, "main");

        {
            ////// INIT
            auto msg = new protos::Msg();
            msg->set_data("helloww");
            msg->set_route("someroute");
            /////
            protos::Request req;
            req.set_allocated_msg(msg);
            std::vector<uint8_t> buffer(req.ByteSizeLong());
            req.SerializeToArray(buffer.data(), buffer.size());
            ///// FINISH
            protos::Response res;

            auto err = Cluster::Instance().RPC("csharp.testremote.remote", req, res);
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
