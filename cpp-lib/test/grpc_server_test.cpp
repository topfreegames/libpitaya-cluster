#include "test_common.h"

#include "pitaya.h"
#include "pitaya/constants.h"
#include "pitaya/grpc/rpc_client.h"
#include "pitaya/grpc/rpc_server.h"
#include "pitaya/protos/pitaya_mock.grpc.pb.h"

#include "mock_service_discovery.h"
#include <cpprest/json.h>
#include <regex>

namespace json = web::json;
using namespace pitaya::service_discovery;
using namespace testing;

class GrpcServerTest : public testing::Test
{
public:
    void SetUp() override
    {
        _config = pitaya::GrpcConfig();
        _config.connectionTimeout = std::chrono::seconds(1);
        _config.host = "localhost";
        _config.port = 58000;

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
        return std::unique_ptr<pitaya::GrpcClient>(new pitaya::GrpcClient(_config, std::shared_ptr<ServiceDiscovery>(_mockSd)));
    }

    void TearDown() override { _client.reset(); }

protected:
    pitaya::GrpcConfig _config;
    pitaya::Server _server;
    std::unique_ptr<pitaya::GrpcClient> _client;
    NiceMock<MockServiceDiscovery>* _mockSd;
};

TEST_F(GrpcServerTest, ServerCanBeCreatedAndDestroyed)
{
    EXPECT_NO_THROW(CreateServer([](const protos::Request& req) -> protos::Response {
        (void)req;
        return protos::Response();
    }));
}

TEST_F(GrpcServerTest, ThrowsIfAddressIsInvalid)
{
    _config.host = "oqijdwioqwjdiowqjdj";
    _config.port = 123123;

    EXPECT_THROW(CreateServer([](const protos::Request& req) -> protos::Response {
                     (void)req;
                     return protos::Response();
                 }),
                 pitaya::PitayaException);
}

TEST_F(GrpcServerTest, CallHandleDoesNotSupportRpcSys)
{
    bool called = false;

    auto server = CreateServer([&](const protos::Request& req) -> protos::Response {
        called = true;
        return protos::Response();
    });

    auto client = CreateClient();
    client->ServerAdded(_server);

    protos::Request req;

    auto res = client->Call(_server, req);

    ASSERT_TRUE(res.has_error());
    ASSERT_FALSE(called);
    EXPECT_EQ(res.error().code(), pitaya::constants::kCodeUnprocessableEntity);
    EXPECT_TRUE(std::regex_search(res.error().msg(), std::regex("does not support RPC Sys")));
}

TEST_F(GrpcServerTest, CallHandleSupportsRpcUser)
{
    bool called = false;

    auto server = CreateServer([&](const protos::Request& req) -> protos::Response {
        called = true;
        EXPECT_EQ(req.msg().route(), "my.custom.route");

        protos::Response res;
        res.set_data("SERVER DATA");
        return res;
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
