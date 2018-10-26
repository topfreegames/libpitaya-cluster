#include "service_discovery.h"
#include <nats/nats.h>
#include <etcd/Client.hpp>

void Test()
{
    etcd::Client etcd("http://127.0.0.1:4001");
    pplx::task<etcd::Response> response_task = etcd.get("/test/key1");
    // ... do something else
    etcd::Response response = response_task.get();
    if (response.is_ok()){
      std::cout << response.value.value << std::endl;
    } else {
      std::cout << "error getting key: " << response.status.etcd_error_message << std::endl;
    }
}
