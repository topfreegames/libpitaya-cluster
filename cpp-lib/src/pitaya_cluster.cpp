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

shared_ptr<protos::Response>
rpc_handler(unique_ptr<protos::Request> req)
{
    cout << "rpc handler called with route: " << req->msg().route() << endl;
    auto res = std::make_shared<protos::Response>();
    auto res2 = protos::Response();
    res2.set_data("ok!");
    std::string serialized;
    res2.SerializeToString(&serialized);
    res->set_data(serialized);
    return res;
}


int
main()
{
    Server server("someid", "sometype");
    NATSConfig nats_config("nats://localhost:4222", 1000, 3000, 3, 100);

    bool init_res = pitaya::Cluster::Instance().Initialize(
        std::move(nats_config), std::move(server), rpc_handler);

    try {
        if (!init_res) {
            throw pitaya::PitayaException("error initializing pitaya cluster");
        }
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

            auto err = pitaya::Cluster::Instance().RPC("sometype.bla.ble", req, res);
            if (err != nullptr) {
                cout << "received error:" << err->msg << endl;
            } else {
                cout << "received answer: " << res->data() << endl;
            }
        }

    } catch (const PitayaException& e) {
        cout << e.what() << endl;
    }

    //    cin >> x;
    pitaya::Cluster::Instance().Shutdown();
    cout << "Shutdown finished!" << endl;
}
