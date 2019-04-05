#include "test_common.h"

#include "pitaya.h"
#include "pitaya/binding_storage.h"
#include "pitaya/constants.h"
#include "pitaya/grpc/rpc_client.h"
#include "pitaya/grpc/rpc_server.h"
#include "pitaya/protos/pitaya_mock.grpc.pb.h"

#include "mock_binding_storage.h"
#include "mock_service_discovery.h"
#include <cpprest/json.h>
#include <regex>

namespace json = web::json;
namespace constants = pitaya::constants;
using namespace pitaya::service_discovery;
using namespace testing;
using pitaya::BindingStorage;

class GrpcServerTest : public testing::Test
{
public:
    void SetUp() override
    {
        _config = pitaya::GrpcConfig();
        _config.connectionTimeout = std::chrono::seconds(1);
        _config.host = "localhost";
        _config.port = 58000;
        _config.serverShutdownDeadline = std::chrono::milliseconds(500);

        _server = pitaya::Server(pitaya::Server::Kind::Frontend, "serverid", "servertype")
                      .WithMetadata(pitaya::constants::kGrpcHostKey, _config.host)
                      .WithMetadata(pitaya::constants::kGrpcPortKey, std::to_string(_config.port));
    }

    std::unique_ptr<pitaya::GrpcServer> CreateServer(pitaya::RpcHandlerFunc handler)
    {
        return std::unique_ptr<pitaya::GrpcServer>(
            new pitaya::GrpcServer(_config, std::move(handler)));
    }

    struct Client
    {
        NiceMock<MockServiceDiscovery>* mockSd;
        MockBindingStorage* mockBs;
        std::unique_ptr<pitaya::GrpcClient> client;
    };

    Client CreateClient()
    {
        Client c;
        c.mockSd = new NiceMock<MockServiceDiscovery>();
        c.mockBs = new MockBindingStorage();
        c.client = std::unique_ptr<pitaya::GrpcClient>(
            new pitaya::GrpcClient(_config,
                                   std::shared_ptr<ServiceDiscovery>(c.mockSd),
                                   std::unique_ptr<BindingStorage>(c.mockBs)));
        return c;
    }

    void TearDown() override { _client.reset(); }

protected:
    pitaya::GrpcConfig _config;
    pitaya::Server _server;
    std::unique_ptr<pitaya::GrpcClient> _client;
};

TEST_F(GrpcServerTest, ServerCanBeCreatedAndDestroyed)
{
    EXPECT_NO_THROW(CreateServer([](const protos::Request& req, pitaya::Rpc* rpc) {
        (void)req;
        rpc->Finish(protos::Response());
    }));
}

TEST_F(GrpcServerTest, ThrowsIfAddressIsInvalid)
{
    _config.host = "oqijdwioqwjdiowqjdj";
    _config.port = 123123;

    EXPECT_THROW(CreateServer([](const protos::Request& req, pitaya::Rpc* rpc) {
                     (void)req;
                     rpc->Finish(protos::Response());
                 }),
                 pitaya::PitayaException);
}

TEST_F(GrpcServerTest, CallHandleDoesSupportRpcSys)
{
    bool called = false;

    auto server = CreateServer([&](const protos::Request& req, pitaya::Rpc* rpc) {
        called = true;
        rpc->Finish(protos::Response());
    });

    NiceMock<ServiceDiscovery>* mockSd;
    MockBindingStorage* mockBs;
    auto c = CreateClient();
    c.client->ServerAdded(_server);

    protos::Request req;

    auto res = c.client->Call(_server, req);

    ASSERT_FALSE(res.has_error());
    ASSERT_TRUE(called);
    EXPECT_EQ(res.data(), "");
}

TEST_F(GrpcServerTest, CallHandleSupportsRpcUser)
{
    bool called = false;

    auto server = CreateServer([&](const protos::Request& req, pitaya::Rpc* rpc) {
        called = true;
        EXPECT_EQ(req.msg().route(), "my.custom.route");

        protos::Response res;
        res.set_data("SERVER DATA");
        rpc->Finish(res);
    });

    auto c = CreateClient();
    c.client->ServerAdded(_server);

    auto msg = new protos::Msg();
    msg->set_route("my.custom.route");

    protos::Request req;
    req.set_type(protos::RPCType::User);
    req.set_allocated_msg(msg);

    auto res = c.client->Call(_server, req);

    ASSERT_FALSE(res.has_error());
    ASSERT_TRUE(called);
    EXPECT_EQ(res.data(), "SERVER DATA");
}

TEST_F(GrpcServerTest, HasGracefulShutdown)
{
    bool called = false;

    auto server = CreateServer([&](const protos::Request& req, pitaya::Rpc* rpc) {
        EXPECT_EQ(req.msg().route(), "my.custom.route");

        auto t = std::thread([rpc, &called]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            protos::Response res;
            res.set_data("SERVER DATA");
            called = true;
            rpc->Finish(res);
        });
        t.detach();
    });

    auto c = CreateClient();
    c.client->ServerAdded(_server);

    auto msg = new protos::Msg();
    msg->set_route("my.custom.route");

    protos::Request req;
    req.set_type(protos::RPCType::User);
    req.set_allocated_msg(msg);

    auto t = std::thread([&]() {
        // Kill the server, right after the call. Since it has graceful shutdown,
        // the server should wait a maximum of _config.serverShutdownDeadline until
        // the remaining RPC is completed.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        server.reset();
    });
    t.detach();

    auto res = c.client->Call(_server, req);
    ASSERT_FALSE(res.has_error());
    EXPECT_TRUE(called);
    EXPECT_EQ(res.data(), "SERVER DATA");

    if (t.joinable()) {
        t.join();
    }
}

TEST_F(GrpcServerTest, HasForcefulShutdownIfDeadlinePasses)
{
    using namespace std::chrono;

    bool called = false;

    auto server = CreateServer([&](const protos::Request& req, pitaya::Rpc* rpc) {
        EXPECT_EQ(req.msg().route(), "my.custom.route");

        auto t = std::thread([rpc, &called]() {
            // Wait longer than _config.serverShutdownDeadline
            std::this_thread::sleep_for(milliseconds(700));
            protos::Response res;
            res.set_data("SERVER DATA");
            called = true;
            rpc->Finish(res);
        });
        t.detach();
    });

    auto c = CreateClient();
    c.client->ServerAdded(_server);

    auto msg = new protos::Msg();
    msg->set_route("my.custom.route");

    protos::Request req;
    req.set_type(protos::RPCType::User);
    req.set_allocated_msg(msg);

    auto t = std::thread([&]() {
        // Kill the server, right after the call. Since it has graceful shutdown,
        // the server should wait a maximum of _config.serverShutdownDeadline until
        // the remaining RPC is completed.
        std::this_thread::sleep_for(milliseconds(50));
        server.reset();
    });

    auto res = c.client->Call(_server, req);
    ASSERT_FALSE(called);
    ASSERT_TRUE(res.has_error());
    EXPECT_EQ(res.error().code(), constants::kCodeInternalError);
    EXPECT_TRUE(std::regex_search(res.error().msg(), std::regex("Call RPC failed")));

    if (t.joinable()) {
        t.join();
    }

    std::this_thread::sleep_for(seconds(1));
}

TEST_F(GrpcServerTest, CanHandleALimitedAmountOfRpcs)
{
    using namespace std::chrono;
    // The server will only handle 2 rpcs at a time.
    _config.serverMaxNumberOfRpcs = 2;

    auto server = CreateServer([&](const protos::Request& req, pitaya::Rpc* rpc) {
        EXPECT_EQ(req.msg().route(), "my.custom.route");

        auto t = std::thread([rpc]() {
            std::this_thread::sleep_for(seconds(1));
            protos::Response res;
            res.set_data("SERVER DATA");
            rpc->Finish(res);
        });
        t.detach();
    });

    std::vector<std::thread> clientThreads(4);
    std::atomic_int numSuccessful(0);
    std::atomic_int numFailures(0);

    for (size_t i = 0; i < clientThreads.size(); ++i) {
        clientThreads[i] = std::thread([this, i, &numSuccessful, &numFailures]() {
            auto c = CreateClient();
            c.client->ServerAdded(_server);

            auto msg = new protos::Msg();
            msg->set_route("my.custom.route");

            protos::Request req;
            req.set_type(protos::RPCType::User);
            req.set_allocated_msg(msg);

            auto res = c.client->Call(_server, req);
            if (res.has_error()) {
                EXPECT_EQ(res.error().code(), constants::kCodeServiceUnavailable);
                numFailures++;
            } else {
                EXPECT_EQ(res.data(), "SERVER DATA");
                numSuccessful++;
            }
        });
    }

    for (auto& thread : clientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    EXPECT_EQ(numSuccessful, _config.serverMaxNumberOfRpcs);
    EXPECT_EQ(numFailures, clientThreads.size() - numSuccessful);
}
