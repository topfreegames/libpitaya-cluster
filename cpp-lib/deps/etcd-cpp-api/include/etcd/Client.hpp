#ifndef __ETCD_CLIENT_HPP__
#define __ETCD_CLIENT_HPP__

#include <etcd/Response.hpp>
#include <etcd/v3/Transaction.hpp>
#include <etcd/v3/AsyncTxnResponse.hpp>
#include <etcd/v3/AsyncKeepAliveAction.hpp>
#include <etcd/v3/Action.hpp>
#include <memory>

#include <string>

#include <grpc++/grpc++.h>
#include <etcd/v3/proto/rpc.grpc.pb.h>

using etcdserverpb::KV;
using etcdserverpb::Watch;
using etcdserverpb::Lease;

using grpc::Channel;

namespace etcd
{
    /**
     * Client is responsible for maintaining a connection towards an etcd server.
     * Etcd operations can be reached via the methods of the client.
     */
    class Client
    {
    public:
        /**
         * Constructs an etcd client object.
         * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:4001"
         * @param taskOptions is the cpprest tasks options (needed for async tasks runtime settings handling)
         */
        Client(std::string const & etcd_url, const pplx::task_options & task_options = pplx::task_options());

        /**
         * Constructs an etcd client object.
         * @param channel is the shared_ptr to gRPC channel, which will be used by client stubs
         * @param taskOptions is the cpprest tasks options (needed for async tasks runtime settings handling)
         */
        Client(std::shared_ptr<Channel>const & channel, const pplx::task_options & task_options = pplx::task_options());

        /**
         * Sends a get request to the etcd server
         * @param key is the key to be read
         */
        pplx::task<Response> get(std::string const & key);

        /**
         * Sets the value of a key. The key will be modified if already exists or created
         * if it does not exists.
         * @param key is the key to be created or modified
         * @param value is the new value to be set
         */
        pplx::task<Response> set(std::string const & key, std::string const & value, int const ttl = 0);

        /**
         * Sets the value of a key. The key will be modified if already exists or created
         * if it does not exists.
         * @param key is the key to be created or modified
         * @param value is the new value to be set
         * @param lease_id is the lease attached to the key
         */
        pplx::task<Response> set(std::string const & key, std::string const & value, int64_t const lease_id);


        /**
         * Creates a new key and sets it's value. Fails if the key already exists.
         * @param key is the key to be created
         * @param value is the value to be set
         */
        pplx::task<Response> add(std::string const & key, std::string const & value, int const ttl = 0);

        /**
         * Creates a new key and sets it's value. Fails if the key already exists.
         * @param key is the key to be created
         * @param value is the value to be set
         * @param lease_id is the lease attached to the key
         */
        pplx::task<Response> add(std::string const & key, std::string const & value, int64_t const lease_id);

        /**
         * Modifies an existing key. Fails if the key does not exists.
         * @param key is the key to be modified
         * @param value is the new value to be set
         */
        pplx::task<Response> modify(std::string const & key, std::string const & value, int const ttl = 0);

        /**
         * Modifies an existing key. Fails if the key does not exists.
         * @param key is the key to be modified
         * @param value is the new value to be set
         * @param lease_id is the lease attached to the key
         */
        pplx::task<Response> modify(std::string const & key, std::string const & value, int64_t const lease_id);

        /**
         * Modifies an existing key only if it has a specific value. Fails if the key does not exists
         * or the original value differs from the expected one.
         * @param key is the key to be modified
         * @param value is the new value to be set
         * @param old_value is the value to be replaced
         */
        pplx::task<Response> modify_if(std::string const & key, std::string const & value, std::string const & old_value, int const ttl = 0);

        /**
         * Modifies an existing key only if it has a specific value. Fails if the key does not exists
         * or the original value differs from the expected one.
         * @param key is the key to be modified
         * @param value is the new value to be set
         * @param old_value is the value to be replaced
         * @param lease_id is the lease attached to the key
         */
        pplx::task<Response> modify_if(std::string const & key, std::string const & value, std::string const & old_value, int64_t const lease_id);

        /**
         * Modifies an existing key only if it has a specific modification index value. Fails if the key
         * does not exists or the modification index of the previous value differs from the expected one.
         * @param key is the key to be modified
         * @param value is the new value to be set
         * @param old_revision is the expected revision of the original value
         */
        pplx::task<Response> modify_if(std::string const & key, std::string const & value, int64_t const old_revision, int const ttl = 0);

        /**
         * Modifies an existing key only if it has a specific modification index value. Fails if the key
         * does not exists or the modification index of the previous value differs from the expected one.
         * @param key is the key to be modified
         * @param value is the new value to be set
         * @param old_revision is the expected revision of the original value
         * @param lease_id is the lease attached to the key
         */
        pplx::task<Response> modify_if(std::string const & key, std::string const & value, int64_t const old_revision, int64_t const lease_id);

        /**
         * Removes a single key. The key has to point to a plain, non directory entry.
         * @param key is the key to be deleted
         */
        pplx::task<Response> rm(std::string const & key);

        /**
         * Removes a single key but only if it has a specific value. Fails if the key does not exists
         * or the its value differs from the expected one.
         * @param key is the key to be deleted
         * @param old_value is the value old value to be compared with
         */
        pplx::task<Response> rm_if(std::string const & key, std::string const & old_value);

        /**
         * Removes an existing key only if it has a specific modification index value. Fails if the key
         * does not exists or the modification index of it differs from the expected one.
         * @param key is the key to be deleted
         * @param old_revision is the expected revision of the existing value
         */
        pplx::task<Response> rm_if(std::string const & key, int64_t const old_revision);

        /**
         * Gets a directory listing of the directory identified by the key.
         * @param key is the key to be listed
         */
        pplx::task<Response> ls(std::string const & key, bool const keysOnly = false);


        /**
         * Removes a directory node. Fails if the parent directory dos not exists or not a directory.
         * @param key is the directory to be created to be listed
         * @param recursive if true then delete a whole subtree, otherwise deletes only an empty directory.
         */
        pplx::task<Response> rmdir(std::string const & key, bool const recursive = false);

        /**
         * Watches for changes of a key or a subtree. Please note that if you watch e.g. "/testdir" and
         * a new key is created, like "/testdir/newkey" then no change happened in the value of
         * "/testdir" so your watch will not detect this. If you want to detect addition and deletion of
         * directory entries then you have to do a recursive watch.
         * @param key is the value or directory to be watched
         * @param recursive if true watch a whole subtree
         */
        pplx::task<Response> watch(std::string const & key, bool const recursive = false);

        /**
         * Watches for changes of a key or a subtree from a specific index. The index value can be in the "past".
         * @param key is the value or directory to be watched
         * @param from_revision the first revision we are interested in
         * @param recursive if true watch a whole subtree
         */
        pplx::task<Response> watch(std::string const & key, int64_t const from_revision, bool const recursive = false);

        /**
         * Grants a lease.
         * @param ttl is the time to live of the lease
         */
        pplx::task<Response> leasegrant(int const ttl);

        /**
         * Keeps a lease alive.
         * @param id is the lease id.
         */
        pplx::task<Response> lease_keep_alive(int64_t const id);

        /**
         * Stops a lease keep alive.
         */
        void stop_lease_keep_alive()
        {
            _keepAliveAction.reset();
        }

        /**
         * Revokes a lease.
         * @param id is the lease id.
         */
        pplx::task<Response> lease_revoke(int64_t const id);

    private:
        const std::unique_ptr<KV::Stub> _stub;
        const std::unique_ptr<Watch::Stub> _watch_service_stub;
        const std::unique_ptr<Lease::Stub> _lease_service_stub;
        const pplx::task_options _task_options;
        std::shared_ptr<etcdv3::AsyncKeepAliveAction> _keepAliveAction;
    };

}

#endif
