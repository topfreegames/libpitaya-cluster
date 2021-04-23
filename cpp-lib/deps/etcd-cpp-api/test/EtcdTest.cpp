#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <iostream>

#include <etcd/Client.hpp>


static std::string z10("http://127.0.0.1:2379");


TEST_CASE("setup")
{
  etcd::Client etcd(z10);
  etcd.rmdir("/test", true).wait();
}


TEST_CASE("add a new key")
{
  
  etcd::Client etcd( z10);
  etcd.rmdir("/test", true).wait();
  etcd::Response resp = etcd.add("/test/key1", "42").get();
  REQUIRE(resp.is_ok());
  CHECK("create" == resp.action);
  etcd::Value const & val = resp.value;
  CHECK("42" == val.value);
  CHECK("/test/key1" == val.key);
  CHECK(!val.is_dir);
  CHECK(0 < val.created_revision);
  CHECK(0 < val.modified_revision);
  CHECK(0 < resp.revision); // X-Etcd-Index header value
  CHECK(etcd::StatusCode::KEY_ALREADY_EXISTS == etcd.add("/test/key1", "43").get().status.etcd_error_code); // Key already exists
  CHECK(etcd::StatusCode::KEY_ALREADY_EXISTS == etcd.add("/test/key1", "42").get().status.etcd_error_code); // Key already exists
  CHECK("Key already exists" == etcd.add("/test/key1", "42").get().status.etcd_error_message);
}


TEST_CASE("read a value from etcd")
{
  etcd::Client etcd( z10);
  etcd::Response resp = etcd.get("/test/key1").get();
  CHECK("get" == resp.action);
  REQUIRE(resp.is_ok());
  CHECK("42" == resp.value.value);
  CHECK("" == etcd.get("/test").get().value.value); // key points to a directory
}


TEST_CASE("simplified read")
{
  etcd::Client etcd( z10);
  CHECK("42" == etcd.get("/test/key1").get().value.value);
  CHECK(etcd::StatusCode::KEY_NOT_FOUND == etcd.get("/test/key2").get().status.etcd_error_code); // Key not found
  CHECK(""  == etcd.get("/test/key2").get().value.value); // Key not found
}



TEST_CASE("modify a key")
{
  etcd::Client etcd( z10);
  etcd::Response resp = etcd.modify("/test/key1", "43").get();
  REQUIRE(resp.is_ok()); // overwrite
  CHECK("update" == resp.action);
  CHECK(etcd::StatusCode::KEY_NOT_FOUND == etcd.modify("/test/key2", "43").get().status.etcd_error_code); // Key not found
  CHECK("43" == etcd.modify("/test/key1", "42").get().prev_value.value);
}


TEST_CASE("set a key")
{
  etcd::Client etcd( z10);
  etcd::Response resp = etcd.set("/test/key1", "43").get();
  REQUIRE(resp.is_ok()); // overwrite
  CHECK("set" == resp.action);
  CHECK(etcd.set("/test/key2", "43").get().is_ok()); // create new
  CHECK("43" == etcd.set("/test/key2", "44").get().prev_value.value);
  CHECK(""   == etcd.set("/test/key3", "44").get().prev_value.value);
  CHECK(etcd.set("/test",      "42").get().is_ok()); // Not a file

  //set with ttl
  resp = etcd.set("/test/key1", "50", 10).get();
  REQUIRE(resp.is_ok()); // overwrite
  CHECK("set" == resp.action);
  CHECK("43" == resp.prev_value.value);
  CHECK("50" == resp.value.value);
  CHECK( 0 < resp.value.lease_id);
}

TEST_CASE("atomic compare-and-swap")
{
  etcd::Client etcd( z10);
  etcd.set("/test/key1", "42").wait();

  // modify success
  etcd::Response res = etcd.modify_if("/test/key1", "43", "42").get();
  int revision = res.revision;
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action);
  CHECK("43" == res.value.value);

  // modify fails the second time
  res = etcd.modify_if("/test/key1", "44", "42").get();
  CHECK(!res.is_ok());
  CHECK(etcd::StatusCode::TEST_FAILED == res.status.etcd_error_code);
  CHECK("Compare failed" == res.status.etcd_error_message);

  // modify fails the second time
  res = etcd.modify_if("/test/key222", "44", "42").get();
  CHECK(!res.is_ok());
  CHECK(etcd::StatusCode::KEY_NOT_FOUND == res.status.etcd_error_code);
  CHECK("Key not found" == res.status.etcd_error_message);

}


TEST_CASE("delete a value")
{
  etcd::Client etcd( z10);
  etcd::Response resp = etcd.rm("/test/key11111").get();
  CHECK(!resp.is_ok());
  CHECK(etcd::StatusCode::KEY_NOT_FOUND == resp.status.etcd_error_code);
  CHECK("Key not found" == resp.status.etcd_error_message);

  int revision = etcd.get("/test/key1").get().revision;
  int create_revision = etcd.get("/test/key1").get().value.created_revision;
  int modify_revision = etcd.get("/test/key1").get().value.modified_revision;
  resp = etcd.rm("/test/key1").get();
  CHECK("43" == resp.prev_value.value);
  CHECK( "/test/key1" == resp.prev_value.key);
  CHECK( create_revision == resp.prev_value.created_revision);
  CHECK( modify_revision == resp.prev_value.modified_revision);
  CHECK("delete" == resp.action);
  CHECK( modify_revision == resp.value.modified_revision);
  CHECK( create_revision == resp.value.created_revision);
  CHECK("" == resp.value.value);
  CHECK( "/test/key1" == resp.value.key);
}

TEST_CASE("atomic compare-and-delete based on prevValue")
{
  etcd::Client etcd( z10);
  etcd.set("/test/key1", "42").wait();
  etcd::Response res = etcd.rm_if("/test/key1", "43").get();
  CHECK(!res.is_ok());
  CHECK(etcd::StatusCode::TEST_FAILED == res.status.etcd_error_code);
  CHECK("Compare failed" == res.status.etcd_error_message);
  res = etcd.rm_if("/test/key1", "42").get();
  REQUIRE(res.is_ok());
  CHECK("compareAndDelete" == res.action);
  CHECK("42" == res.prev_value.value);
}

TEST_CASE("atomic compare-and-delete based on prevIndex")
{
  etcd::Client etcd( z10);
  int revision = etcd.set("/test/key1", "42").get().revision;
  etcd::Response res = etcd.rm_if("/test/key1", revision - 1).get();
  CHECK(!res.is_ok());
  CHECK(etcd::StatusCode::TEST_FAILED == res.status.etcd_error_code);
  CHECK("Compare failed" == res.status.etcd_error_message);

  res = etcd.rm_if("/test/key1", revision).get();
  REQUIRE(res.is_ok());
  CHECK("compareAndDelete" == res.action);
  CHECK("42" == res.prev_value.value);
}


TEST_CASE("deep atomic compare-and-swap")
{
  etcd::Client etcd( z10);
  etcd.set("/test/key1", "42").wait();

  // modify success
  etcd::Response res = etcd.modify_if("/test/key1", "43", "42").get();
  int revision = res.revision;
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action);
  CHECK("43" == res.value.value);

  // modify fails the second time
  res = etcd.modify_if("/test/key1", "44", "42").get();
  CHECK(!res.is_ok());
  CHECK(etcd::StatusCode::TEST_FAILED == res.status.etcd_error_code);
  CHECK("Compare failed" == res.status.etcd_error_message);


  // succes with the correct revision
  res = etcd.modify_if("/test/key1", "44", revision).get();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action);
  CHECK("44" == res.value.value);

  // revision changes so second modify fails
  res = etcd.modify_if("/test/key1", "45", revision).get();
  CHECK(!res.is_ok());
  CHECK(etcd::StatusCode::TEST_FAILED == res.status.etcd_error_code);
  CHECK("Compare failed" == res.status.etcd_error_message);

}


//skip this test case
/*
TEST_CASE("create a directory")
{
  etcd::Client etcd("http://127.0.0.1:4001");
  etcd::Response resp = etcd.mkdir("/test/new_dir").get();
  CHECK("set" == resp.action);
  CHECK(resp.value.is_dir);
}
*/

TEST_CASE("list a directory")
{
  etcd::Client etcd( z10);
  CHECK(0 == etcd.ls("/test/new_dir").get().keys.size());

  etcd.set("/test/new_dir/key1", "value1").wait();
  etcd::Response resp = etcd.ls("/test/new_dir").get();
  CHECK("get" == resp.action);
  REQUIRE(1 == resp.keys.size());
  REQUIRE(1 == resp.values.size());
  CHECK("/test/new_dir/key1" == resp.keys[0]);
  CHECK("value1" == resp.values[0].value);
  CHECK(resp.values[0].value == "value1");

  etcd.set("/test/new_dir/key2", "value2").wait();
  etcd.set("/test/new_dir/sub_dir", "value3").wait();

  resp = etcd.ls("/test/new_dir").get();
  CHECK("get" == resp.action);
  REQUIRE(3 == resp.keys.size());
  CHECK("/test/new_dir/key1" == resp.keys[0]);
  CHECK("/test/new_dir/key2" == resp.keys[1]);
  CHECK("/test/new_dir/sub_dir" == resp.keys[2]);
  CHECK("value1" == resp.values[0].value);
  CHECK("value2" == resp.values[1].value);
  CHECK(resp.values[2].is_dir == 0);

  CHECK(etcd.ls("/test/new_dir/key1").get().is_ok());
}

TEST_CASE("delete a directory")
{
  etcd::Client etcd( z10);

  //CHECK(108 == etcd.rmdir("/test/new_dir").get().status.etcd_error_code); // Directory not empty
  etcd::Response resp = etcd.ls("/test/new_dir").get();


  resp = etcd.rmdir("/test/new_dir", true).get();
  int revision = resp.revision;
  CHECK("delete" == resp.action);
  REQUIRE(3 == resp.keys.size());
  CHECK("/test/new_dir/key1" == resp.keys[0]);
  CHECK("/test/new_dir/key2" == resp.keys[1]);
  CHECK("value1" == resp.values[0].value);
  CHECK("value2" == resp.values[1].value);


  resp = etcd.rmdir("/test/dirnotfound", true).get();
  CHECK(!resp.is_ok());
  CHECK(etcd::StatusCode::KEY_NOT_FOUND == resp.status.etcd_error_code);
  CHECK("Key not found" == resp.status.etcd_error_message);

  resp = etcd.rmdir("/test/new_dir", false).get();
  CHECK(!resp.is_ok());
  CHECK(etcd::StatusCode::KEY_NOT_FOUND == resp.status.etcd_error_code);
  CHECK("Key not found" == resp.status.etcd_error_message);
}

TEST_CASE("wait for a value change")
{
  etcd::Client etcd( z10);
  etcd.set("/test/key1", "42").wait();

  pplx::task<etcd::Response> res = etcd.watch("/test/key1");
  CHECK(!res.is_done());
  sleep(1);
  CHECK(!res.is_done());
  
  etcd.set("/test/key1", "43").get();
  sleep(1);
  REQUIRE(res.is_done());
  REQUIRE("set" == res.get().action);
  CHECK("43" == res.get().value.value);
  CHECK("42" == res.get().prev_value.value);
}

TEST_CASE("wait for a directory change")
{
  etcd::Client etcd( z10);

  pplx::task<etcd::Response> res = etcd.watch("/test", true);
  CHECK(!res.is_done());
  sleep(1);
  CHECK(!res.is_done());

  etcd.add("/test/key4", "44").wait();
  sleep(1);
  REQUIRE(res.is_done());
  CHECK("create" == res.get().action);
  CHECK("44" == res.get().value.value);

  pplx::task<etcd::Response> res2 = etcd.watch("/test", true);
  CHECK(!res2.is_done());
  sleep(1);
  CHECK(!res2.is_done());

  etcd.set("/test/key4", "45").wait();
  sleep(1);
  REQUIRE(res2.is_done());
  CHECK("set" == res2.get().action);
  CHECK("45" == res2.get().value.value);
}

TEST_CASE("watch changes in the past")
{
  etcd::Client etcd( z10);
  REQUIRE(etcd.rmdir("/test", true).get().is_ok());
  int64_t revision = etcd.set("/test/key1", "42").get().revision;

  etcd.set("/test/key1", "43").wait();
  etcd.set("/test/key1", "44").wait();
  etcd.set("/test/key1", "45").wait();

  etcd::Response res = etcd.watch("/test/key1", ++revision).get();
  CHECK("set" == res.action);
  // we`re looking for last change made since specific revision
  CHECK("45" == res.value.value);
  CHECK("44" == res.prev_value.value);

  res = etcd.watch("/test/key1", ++revision).get();
  CHECK("set" == res.action);
  // we`re looking for last change made since specific revision
  // TODO: all events should be stored in values
  CHECK("45" == res.value.value);

  res = etcd.watch("/test", ++revision, true).get();
  CHECK("set" == res.action);
  CHECK("45" == res.value.value);
}

TEST_CASE("lease grant")
{
  etcd::Client etcd( z10);
  etcd::Response res = etcd.leasegrant(60).get();
  REQUIRE(res.is_ok());
  CHECK(60 == res.value.ttl);
  CHECK(0 <  res.value.lease_id);
  int64_t leaseid = res.value.lease_id;

  res = etcd.set("/test/key1", "43", leaseid).get();
  REQUIRE(res.is_ok()); // overwrite
  CHECK("set" == res.action);
  CHECK(leaseid ==  res.value.lease_id);

  //change leaseid
  res = etcd.leasegrant(10).get();
  leaseid = res.value.lease_id;
  res = etcd.set("/test/key1", "43", leaseid).get();
  REQUIRE(res.is_ok()); // overwrite
  CHECK("set" == res.action);
  CHECK(leaseid == res.value.lease_id);

  //failure to attach lease id
  res = etcd.set("/test/key1", "43", leaseid+1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(grpc::StatusCode::NOT_FOUND == res.status.grpc_error_code);
  CHECK("etcdserver: requested lease not found" == res.status.grpc_error_message);

  res = etcd.modify("/test/key1", "44", leaseid).get();
  REQUIRE(res.is_ok()); // overwrite
  CHECK("update" == res.action);
  CHECK(leaseid ==  res.value.lease_id);
  CHECK("44" ==  res.value.value);

  //failure to attach lease id
  res = etcd.modify("/test/key1", "45", leaseid+1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(grpc::StatusCode::NOT_FOUND == res.status.grpc_error_code);
  CHECK("etcdserver: requested lease not found" == res.status.grpc_error_message);

  res = etcd.modify_if("/test/key1", "45", "44", leaseid).get();
  int revision = res.revision;
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action);
  CHECK("45" == res.value.value);

  //failure to attach lease id
  res = etcd.modify_if("/test/key1", "46", "45", leaseid+1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(grpc::StatusCode::NOT_FOUND == res.status.grpc_error_code);
  CHECK("etcdserver: requested lease not found" == res.status.grpc_error_message);

  // succes with the correct revision
  res = etcd.modify_if("/test/key1", "44", revision, leaseid).get();
  revision = res.revision;
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action);
  CHECK("44" == res.value.value);

  res = etcd.modify_if("/test/key1", "44", revision, leaseid+1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(grpc::StatusCode::NOT_FOUND == res.status.grpc_error_code);
  CHECK("etcdserver: requested lease not found" == res.status.grpc_error_message);

  res = etcd.add("/test/key11111", "43", leaseid).get();
  REQUIRE(res.is_ok());
  CHECK("create" == res.action);
  CHECK(leaseid ==  res.value.lease_id);

  //failure to attach lease id
  res = etcd.set("/test/key11111", "43", leaseid+1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(grpc::StatusCode::NOT_FOUND == res.status.grpc_error_code);
  CHECK("etcdserver: requested lease not found" == res.status.grpc_error_message);
}

TEST_CASE("cleanup")
{
  etcd::Client etcd( z10);
  REQUIRE(etcd.rmdir("/test", true).get().is_ok());
}


