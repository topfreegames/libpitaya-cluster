#include <memory>
#include <etcd/Utils.hpp>
#include <etcd/Client.hpp>
#include <etcd/v3/AsyncTxnResponse.hpp>
#include <etcd/v3/AsyncRangeResponse.hpp>
#include <etcd/v3/AsyncWatchResponse.hpp>
#include <etcd/v3/AsyncDeleteRangeResponse.hpp>
#include <etcd/v3/Transaction.hpp>
#include <etcd/v3/AsyncKeepAliveAction.hpp>
#include <etcd/v3/AsyncKeepAliveResponse.hpp>
#include <etcd/v3/AsyncLeaseRevokeAction.hpp>
#include <iostream>

#include <etcd/v3/AsyncSetAction.hpp>
#include <etcd/v3/AsyncCompareAndSwapAction.hpp>
#include <etcd/v3/AsyncCompareAndDeleteAction.hpp>
#include <etcd/v3/AsyncUpdateAction.hpp>
#include <etcd/v3/AsyncGetAction.hpp>
#include <etcd/v3/AsyncDeleteAction.hpp>
#include <etcd/v3/AsyncWatchAction.hpp>
#include <etcd/v3/AsyncLeaseGrantAction.hpp>


using grpc::Channel;


etcd::Client::Client(std::string const & etcd_url, const pplx::task_options & task_options)
  : Client(etcd::utils::createChannel(etcd_url), task_options)
{}

etcd::Client::Client(std::shared_ptr<Channel>const & channel, const pplx::task_options & task_options)
  : _stub(KV::NewStub(channel))
  , _watch_service_stub(Watch::NewStub(channel))
  , _lease_service_stub(Lease::NewStub(channel))
  , _task_options(task_options)
{}

pplx::task<etcd::Response> etcd::Client::get(std::string const & key)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = false;
  params.kv_stub = _stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncGetAction>(std::move(params)), _task_options);
}

pplx::task<etcd::Response> etcd::Client::set(std::string const & key, std::string const & value, int const ttl)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.kv_stub = _stub.get();

  if(ttl > 0)
  {
    auto res = leasegrant(ttl).get();
    if(!res.is_ok())
    {
      auto status = std::move(res.status);
      return pplx::task<etcd::Response>([status]()
      {
        return etcd::Response(status);
      }, _task_options);
    }
    else
    {
      params.lease_id = res.value.lease_id;
    }
  }
  return Response::create(std::make_shared<etcdv3::AsyncSetAction>(std::move(params)), _task_options);
}

pplx::task<etcd::Response> etcd::Client::set(std::string const & key, std::string const & value, int64_t const lease_id)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = lease_id;
  params.kv_stub = _stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncSetAction>(std::move(params)), _task_options);
}


pplx::task<etcd::Response> etcd::Client::add(std::string const & key, std::string const & value, int const ttl)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.kv_stub = _stub.get();

  if(ttl > 0)
  {
    auto res = leasegrant(ttl).get();
    if(!res.is_ok())
    {
      auto status = std::move(res.status);
      return pplx::task<etcd::Response>([status]()
      {
        return etcd::Response(status);
      }, _task_options);
    }
    else
    {
      params.lease_id = res.value.lease_id;
    }
  }
  return Response::create(std::make_shared<etcdv3::AsyncSetAction>(std::move(params), true), _task_options);
}

pplx::task<etcd::Response> etcd::Client::add(std::string const & key, std::string const & value, int64_t const lease_id)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = lease_id;
  params.kv_stub = _stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncSetAction>(std::move(params), true), _task_options);
}


pplx::task<etcd::Response> etcd::Client::modify(std::string const & key, std::string const & value, int const ttl)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.kv_stub = _stub.get();

  if(ttl > 0)
  {
    auto res = leasegrant(ttl).get();
    if(!res.is_ok())
    {
      auto status = std::move(res.status);
      return pplx::task<etcd::Response>([status]()
      {
        return etcd::Response(status);
      }, _task_options);
    }
    else
    {
      params.lease_id = res.value.lease_id;
    }
  }
  return Response::create(std::make_shared<etcdv3::AsyncUpdateAction>(std::move(params)), _task_options);
}

pplx::task<etcd::Response> etcd::Client::modify(std::string const & key, std::string const & value, int64_t const lease_id)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = lease_id;
  params.kv_stub = _stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncUpdateAction>(std::move(params)), _task_options);
}


pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, std::string const & old_value, int const ttl)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.old_value.assign(old_value);
  params.kv_stub = _stub.get();

  if(ttl > 0)
  {
    auto res = leasegrant(ttl).get();
    if(!res.is_ok())
    {
      auto status = std::move(res.status);
      return pplx::task<etcd::Response>([status]()
      {
        return etcd::Response(status);
      }, _task_options);
    }
    else
    {
      params.lease_id = res.value.lease_id;
    }
  }
  return Response::create(std::make_shared<etcdv3::AsyncCompareAndSwapAction>(std::move(params), etcdv3::Atomicity_Type::PREV_VALUE), _task_options);
}

pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, std::string const & old_value, int64_t const lease_id)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.old_value.assign(old_value);
  params.lease_id = lease_id;
  params.kv_stub = _stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncCompareAndSwapAction>(std::move(params), etcdv3::Atomicity_Type::PREV_VALUE), _task_options);
}



pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, int64_t const old_revision, int const ttl)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.old_revision = old_revision;
  params.kv_stub = _stub.get();
  if(ttl > 0)
  {
    auto res = leasegrant(ttl).get();
    if(!res.is_ok())
    {
      auto status = std::move(res.status);
      return pplx::task<etcd::Response>([status]()
      {
        return etcd::Response(status);
      }, _task_options);
    }
    else
    {
      params.lease_id = res.value.lease_id;
    }
  }
  return Response::create(std::make_shared<etcdv3::AsyncCompareAndSwapAction>(std::move(params), etcdv3::Atomicity_Type::PREV_INDEX), _task_options);
}

pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, int64_t const old_revision, int64_t const lease_id)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = lease_id;
  params.old_revision = old_revision;
  params.kv_stub = _stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncCompareAndSwapAction>(std::move(params), etcdv3::Atomicity_Type::PREV_INDEX), _task_options);
}


pplx::task<etcd::Response> etcd::Client::rm(std::string const & key)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = false;
  params.kv_stub = _stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncDeleteAction>(std::move(params)), _task_options);
}


pplx::task<etcd::Response> etcd::Client::rm_if(std::string const & key, std::string const & old_value)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.old_value.assign(old_value);
  params.kv_stub = _stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncCompareAndDeleteAction>(std::move(params), etcdv3::Atomicity_Type::PREV_VALUE), _task_options);
}

pplx::task<etcd::Response> etcd::Client::rm_if(std::string const & key, int64_t const old_revision)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.old_revision = old_revision;
  params.kv_stub = _stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncCompareAndDeleteAction>(std::move(params), etcdv3::Atomicity_Type::PREV_INDEX), _task_options);

}

pplx::task<etcd::Response> etcd::Client::rmdir(std::string const & key, bool const recursive)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = recursive;
  params.kv_stub = _stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncDeleteAction>(std::move(params)), _task_options);
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const & key, bool const keysOnly)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = true;
  params.keysOnly = keysOnly;
  params.kv_stub = _stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncGetAction>(std::move(params)), _task_options);
}

// TODO: remove watch from client, rename to Client to KVClient
pplx::task<etcd::Response> etcd::Client::watch(std::string const & key, bool const recursive)
{
  return watch(key, 0, recursive);
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const & key, int64_t const from_revision, bool const recursive)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = recursive;
  params.revision = from_revision;
  params.watch_stub = _watch_service_stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncWatchAction>(std::move(params)), _task_options);
}

pplx::task<etcd::Response> etcd::Client::leasegrant(int const ttl)
{
  etcdv3::ActionParameters params;
  params.ttl = ttl;
  params.lease_stub = _lease_service_stub.get();
  return Response::create(std::make_shared<etcdv3::AsyncLeaseGrantAction>(std::move(params)), _task_options);
}

pplx::task<etcd::Response> etcd::Client::lease_revoke(int64_t const id)
{
    etcdv3::ActionParameters params;
    params.lease_id = id;
    params.lease_stub = _lease_service_stub.get();
    return Response::create(std::make_shared<etcdv3::AsyncLeaseRevokeAction>(std::move(params)), _task_options);
}

pplx::task<etcd::Response> etcd::Client::lease_keep_alive(int64_t const id)
{
    if (_keepAliveAction == nullptr) {
        etcdv3::ActionParameters params;
        params.lease_id = id;
        params.lease_stub = _lease_service_stub.get();

        _keepAliveAction = std::make_shared<etcdv3::AsyncKeepAliveAction>(std::move(params));
    }

    _keepAliveAction->setLeaseId(id);

    return Response::create(_keepAliveAction, _task_options);
}

