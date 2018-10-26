#include "pitaya_cluster.h"
#include <nats/nats.h>
//#include <grpc/grpc.h>
//#include <grpc++/grpc++.h>
#include <etcd/Client.hpp>

void Test()
{
    etcd::Client etcd("http://127.0.0.1:4001");
    pplx::task<etcd::Response> response_task = etcd.get("/test/key1");
    std::cout << "WILL CALL ETCD" << std::endl;
    // ... do something else
    etcd::Response response = response_task.get();
    std::cout << response.value.value << std::endl;
}
