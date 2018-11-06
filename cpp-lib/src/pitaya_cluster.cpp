#include "pitaya.h"
#include "pitaya/cluster.h"
#include "pitaya/service_discovery.h"
#include "protos/msg.pb.h"
#include "protos/request.pb.h"
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
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
    Server server("someid", "sometype");
    NATSConfig nats_config("nats://localhost:4222", 1000, 3000, 3, 100);

    service_discovery::Config sdConfig;
    sdConfig.endpoints = "http://127.0.0.1:4001";
    sdConfig.etcdPrefix = "pitaya/servers";


    try {
        Cluster cluster(std::move(sdConfig), std::move(nats_config), std::move(server), rpc_handler);

        {
            ////// INIT
            auto msg = new protos::Msg();
            msg->set_data("helloww");
            msg->set_route("someroute");
            /////
            auto req = std::make_shared<protos::Request>();
            req->set_allocated_msg(msg);
            size_t size = req->ByteSizeLong();
            void* buffer = malloc(size);
            req->SerializeToArray(buffer, size);
            ///// FINISH
            auto res = std::make_shared<protos::Response>();

            auto err = cluster.RPC("csharp.testremote.remote", req, res);
            if (err != nullptr) {
                cout << "received error:" << err->msg << endl;
            } else {
                cout << "received answer: " << res->data() << endl;
            }
        }

        cin >> x;
    } catch (const PitayaException& e) {
        cout << e.what() << endl;
        cin >> x;
    }
}
