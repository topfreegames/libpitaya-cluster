#include <catch.hpp>

#include <unistd.h>

#include <etcd/Watcher.hpp>
#include <etcd/SyncClient.hpp>

static std::string etcd_uri("http://127.0.0.1:2379");
static int watcher_called = 0;

void printResponse(etcd::Response && resp)
{
  ++watcher_called;
  std::cout << "print response called" << std::endl;
  if (!resp.is_ok()) {
    std::cout << "grpc error:\n" << (int)resp.status.grpc_error_code << ": " << resp.status.grpc_error_message << "\n"
              << "etcd error:\n" << (int)resp.status.etcd_error_code << ": " << resp.status.etcd_error_message << std::endl;
  }
  else
  {
    auto action = std::move(resp.action);
    auto value = std::move(resp.value.value);
    std::cout << "Action: " << action << ", value: " << value << std::endl;
    std::cout << "Previous value: " << resp.prev_value.value << std::endl;
  }
}

TEST_CASE("create watcher with cancel")
{
  
  etcd::SyncClient etcd(etcd_uri);
  etcd.rmdir("/test", true);

  watcher_called = 0;
  etcd::Watcher watcher(etcd_uri, "/test", printResponse);
  sleep(1);
  etcd.set("/test/key", "42");
  etcd.set("/test/key", "43");
  etcd.rm("/test/key");
  etcd.set("/test/key", "44");
  sleep(1);
  CHECK(4 == watcher_called);
  watcher.cancel();
  etcd.set("/test/key", "50");
  etcd.set("/test/key", "51");
  sleep(1);
  CHECK(4 == watcher_called);

  etcd.rmdir("/test", true);

}

TEST_CASE("create watcher")
{
  
  etcd::SyncClient etcd(etcd_uri);
  etcd.rmdir("/test", true);
  etcd.set("/test/key", "42");

  watcher_called = 0;
  {
    etcd::Watcher watcher(etcd_uri, "/test", printResponse);
    sleep(1);
    etcd.set("/test/key", "42");
    etcd.set("/test/key", "43");
  }
  
  CHECK(2 == watcher_called);
// TEST_CASE("wait for a value change")
// {
//   etcd::Client etcd(etcd_uri);
//   etcd.set("/test/key1", "42").wait();

//   pplx::task<etcd::Response> res = etcd.watch("/test/key1");
//   CHECK(!res.is_done());

//   etcd.set("/test/key1", "43").get();
//   sleep(1);

//   REQUIRE(res.is_done());
//   REQUIRE("set" == res.get().action);
//   CHECK("43" == res.get().value.value);
// }

// TEST_CASE("wait for a directory change")
// {
//   etcd::Client etcd(etcd_uri);

//   pplx::task<etcd::Response> res = etcd.watch("/test", true);

//   etcd.add("/test/key4", "44").wait();
//   REQUIRE(res.is_done());
//   CHECK("create" == res.get().action);
//   CHECK("44" == res.get().value.value);

//   pplx::task<etcd::Response> res2 = etcd.watch("/test", true);

//   etcd.set("/test/key4", "45").wait();
//   sleep(1);
//   REQUIRE(res2.is_done());
//   CHECK("set" == res2.get().action);
//   CHECK("45" == res2.get().value.value);
// }

// TEST_CASE("watch changes in the past")
// {
//   etcd::Client etcd(etcd_uri);

//   int index = etcd.set("/test/key1", "42").get().index();

//   etcd.set("/test/key1", "43").wait();
//   etcd.set("/test/key1", "44").wait();
//   etcd.set("/test/key1", "45").wait();

//   etcd::Response res = etcd.watch("/test/key1", ++index).get();
//   CHECK("set" == res.action);
//   CHECK("43" == res.value.value);

//   res = etcd.watch("/test/key1", ++index).get();
//   CHECK("set" == res.action);
//   CHECK("44" == res.value.value);

//   res = etcd.watch("/test", ++index, true).get();
//   CHECK("set" == res.action);
//   CHECK("45" == res.value.value);
// }

// TEST_CASE("request cancellation")
// {
//   etcd::Client etcd(etcd_uri);
//   etcd.set("/test/key1", "42").wait();

//   pplx::task<etcd::Response> res = etcd.watch("/test/key1");
//   CHECK(!res.is_done());

//   etcd.cancel_operations();

//   sleep(1);
//   REQUIRE(res.is_done());
//   try
//   {
//     res.wait();
//   }
//   catch(pplx::task_canceled const & ex)
//   {
//     std::cout << "pplx::task_canceled: " << ex.what() << "\n";
//   }
//   catch(std::exception const & ex)
//   {
//     std::cout << "std::exception: " << ex.what() << "\n";
//   }
// }
   CHECK(etcd.rmdir("/test", true).is_ok());
}
