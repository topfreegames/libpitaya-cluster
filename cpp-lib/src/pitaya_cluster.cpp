#include <iostream>
#include <memory>
#include <service_discovery.h>
#include "protos/msg.pb.h"
#include "protos/request.pb.h"
#include <pitaya.h>
#include <cluster.h>
#include <pitaya_nats.h>
#include <exception>

using namespace std;
using namespace pitaya_nats;
using namespace pitaya;
using service_discovery::ServiceDiscovery;

unique_ptr<ServiceDiscovery> gServiceDiscovery;
unique_ptr<NATSRPCServer> nats_rpc_server;
unique_ptr<NATSRPCClient> nats_rpc_client;
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

int main()
{
    Server server("someid", "sometype");
    NATSConfig nats_config("nats://localhost:4222", 1000, 3000, 3, 100);

    try {
        auto pit_cluster = unique_ptr<pitaya::Cluster>(new pitaya::Cluster(
            nats_config,
            std::move(server),
            rpc_handler
        ));

        bool init_res = pit_cluster->Init();
        if(!init_res){
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
            void *buffer = malloc(size);
            req->SerializeToArray(buffer, size);
            ///// FINISH
            auto res = std::make_shared<protos::Response>();

            auto err = pit_cluster->RPC("sometype.bla.ble", req, res);
            if (err != nullptr){
                cout << "received error:" << err->msg << endl;
            } else{
                cout << "received answer: " << res->data() << endl;
            }
        }

        cout << "enter a key to exit..." << endl;
        cin >> x;
    } catch (const PitayaException &e){
        cout << e.what() << endl;

        cout << "enter a key to exit..." << endl;
        cin >> x;
    }
}
