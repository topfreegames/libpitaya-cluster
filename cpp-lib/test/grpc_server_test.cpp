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

    std::unique_ptr<pitaya::GrpcClient> CreateClient()
    {
        _mockSd = new NiceMock<MockServiceDiscovery>();
        _mockBs = new MockBindingStorage();
        return std::unique_ptr<pitaya::GrpcClient>(
            new pitaya::GrpcClient(_config,
                                   std::shared_ptr<ServiceDiscovery>(_mockSd),
                                   std::unique_ptr<BindingStorage>(_mockBs)));
    }

    void TearDown() override { _client.reset(); }

protected:
    pitaya::GrpcConfig _config;
    pitaya::Server _server;
    std::unique_ptr<pitaya::GrpcClient> _client;
    NiceMock<MockServiceDiscovery>* _mockSd;
    MockBindingStorage* _mockBs;
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

    auto client = CreateClient();
    client->ServerAdded(_server);

    protos::Request req;

    auto res = client->Call(_server, req);

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

    auto client = CreateClient();
    client->ServerAdded(_server);

    auto msg = new protos::Msg();
    msg->set_route("my.custom.route");

    protos::Request req;
    req.set_type(protos::RPCType::User);
    req.set_allocated_msg(msg);

    auto res = client->Call(_server, req);

    ASSERT_FALSE(res.has_error());
    ASSERT_TRUE(called);
    EXPECT_EQ(res.data(), "SERVER DATA");
}

TEST_F(GrpcServerTest, HasGracefulShutdown)
{
    bool called = false;

    auto server = CreateServer([&](const protos::Request& req, pitaya::Rpc* rpc) {
        EXPECT_EQ(req.msg().route(), "my.custom.route");

        std::thread([rpc, &called]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            protos::Response res;
            res.set_data("SERVER DATA");
            called = true;
            rpc->Finish(res);
        }).detach();
    });

    auto client = CreateClient();
    client->ServerAdded(_server);

    auto msg = new protos::Msg();
    msg->set_route("my.custom.route");

    protos::Request req;
    req.set_type(protos::RPCType::User);
    req.set_allocated_msg(msg);

    std::thread([&]() {
        // Kill the server, right after the call. Since it has graceful shutdown,
        // the server should wait a maximum of _config.serverShutdownDeadline until
        // the remaining RPC is completed.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        server.reset();
    }).detach();

    auto res = client->Call(_server, req);
    ASSERT_FALSE(res.has_error());
    EXPECT_TRUE(called);
    EXPECT_EQ(res.data(), "SERVER DATA");
}

TEST_F(GrpcServerTest, HasForcefulShutdownIfDeadlinePasses)
{
    using namespace std::chrono;

    bool called = false;

    auto server = CreateServer([&](const protos::Request& req, pitaya::Rpc* rpc) {
        EXPECT_EQ(req.msg().route(), "my.custom.route");

        std::thread([rpc, &called]() {
            // Wait longer than _config.serverShutdownDeadline
            std::this_thread::sleep_for(milliseconds(700));
            protos::Response res;
            res.set_data("SERVER DATA");
            called = true;
            rpc->Finish(res);
        }).detach();
    });

    auto client = CreateClient();
    client->ServerAdded(_server);

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

    auto res = client->Call(_server, req);
    ASSERT_FALSE(called);
    ASSERT_TRUE(res.has_error());
    EXPECT_EQ(res.error().code(), constants::kCodeInternalError);
    EXPECT_TRUE(std::regex_search(res.error().msg(), std::regex("Call RPC failed")));

    if (t.joinable()) {
        t.join();
    }

    std::this_thread::sleep_for(seconds(1));
}
