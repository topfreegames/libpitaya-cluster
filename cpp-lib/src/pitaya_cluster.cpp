#include <iostream>
#include <memory>
#include <service_discovery.h>
#include "protos/msg.pb.h"
#include "protos/request.pb.h"
#include <pitaya.h>
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
    res->set_data("ok!");
    return res;
}

int main()
{
    auto server = make_shared<Server>(Server("someid", "sometype"));
    auto nats_config = make_shared<NATSConfig>(NATSConfig("nats://localhost:4222", 1000));

    gServiceDiscovery = std::unique_ptr<ServiceDiscovery>(
        new ServiceDiscovery(server, "http://127.0.0.1:4001")
    );

    try {
        nats_rpc_server = unique_ptr<NATSRPCServer>(
            new NATSRPCServer(server, nats_config, rpc_handler));
        nats_rpc_client = unique_ptr<NATSRPCClient>(
            new NATSRPCClient(server, nats_config));

        // TODO refactor route
        {
            ////// INIT
            auto msg = new protos::Msg();
            msg->set_data("helloww");
            msg->set_route("someroute");
            /////
            auto req = std::unique_ptr<protos::Request>(new protos::Request());
            req->set_allocated_msg(msg);
            size_t size = req->ByteSizeLong(); 
            void *buffer = malloc(size);
            req->SerializeToArray(buffer, size);
            ///// FINISH

            auto res = nats_rpc_client->Call(server, std::move(req));
            if (res->has_error()){
                cout << "received error:" << res->error().msg() << endl;
            } else{
                cout << "received answer: " << res->data() << endl;
            }
        }
    } catch (PitayaException* e){
        cout << e->what() << endl;
    }

    cout << "enter a key to exit..." << endl;
    cin >> x;
}
