#include "test_common.h"

#include "pitaya.h"
#include "pitaya/constants.h"
#include "pitaya/grpc/rpc_client.h"

#include "mock_service_discovery.h"
#include "mock_etcd_client.h"

#include <regex>

using namespace testing;
using namespace pitaya::service_discovery;

class GrpcClientTest : public testing::Test
{
public:
    void SetUp() override
    {
        _mockSd = new MockServiceDiscovery();
        _sd = std::shared_ptr<ServiceDiscovery>(_mockSd);

        _config = pitaya::GrpcConfig();
        _config.connectionTimeout = std::chrono::seconds(2);
        _config.host = "localhost";
        _config.port = 3030;

        // NOTE: since the argument is `this`, we cannot know what `this` points to
        // before creating the client object, therefore this expectation is sub-optimal.
        EXPECT_CALL(*_mockSd, AddListener(_));
        EXPECT_CALL(*_mockSd, RemoveListener(_));

        _client = std::unique_ptr<pitaya::GrpcClient>(new pitaya::GrpcClient(_config, _sd));
    }

    void TearDown() override
    {
        _client.reset();
        _sd.reset();
    }

protected:
    MockServiceDiscovery* _mockSd;
    std::shared_ptr<ServiceDiscovery> _sd;
    pitaya::GrpcConfig _config;
    std::unique_ptr<pitaya::GrpcClient> _client;
};

TEST_F(GrpcClientTest, CallFailsWhenNoConnectionsExist)
{
    pitaya::Server target(pitaya::Server::Kind::Frontend, "myid", "mytype");
    protos::Request req;

    auto res = _client->Call(target, req);
    ASSERT_TRUE(res.has_error());
    EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
    EXPECT_FALSE(res.error().msg().empty());
}

TEST_F(GrpcClientTest, ServersWithoutGrpcSupportAreIgnored)
{
    _config.connectionTimeout = std::chrono::seconds(1);

    {
        pitaya::Server server(pitaya::Server::Kind::Frontend, "server-id", "server-type");
        _client->ServerAdded(server);

        protos::Request req;

        auto res = _client->Call(server, req);
        ASSERT_TRUE(res.has_error());
        EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
        EXPECT_TRUE(std::regex_search(res.error().msg(), std::regex("is not added to the connections map")));
    }
    {
        auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
            .AddMetadata(pitaya::constants::kGrpcHostKey, "host");

        _client->ServerAdded(server);

        protos::Request req;
        auto res = _client->Call(server, req);
        ASSERT_TRUE(res.has_error());
        EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
        EXPECT_TRUE(std::regex_search(res.error().msg(), std::regex("is not added to the connections map")));
    }
    {
        auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
            .AddMetadata(pitaya::constants::kGrpcPortKey, "3030");

        _client->ServerAdded(server);

        protos::Request req;
        auto res = _client->Call(server, req);
        ASSERT_TRUE(res.has_error());
        EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
        EXPECT_TRUE(std::regex_search(res.error().msg(), std::regex("is not added to the connections map")));
    }
}

TEST_F(GrpcClientTest, RemovingUnexistentServersIsIgnored)
{
    pitaya::Server server(pitaya::Server::Kind::Frontend, "server-id", "server-type");
    _client->ServerRemoved(server);
}

TEST_F(GrpcClientTest, ServersThatFailToConnectAreNotIgnored)
{
    _config.connectionTimeout = std::chrono::seconds(1);

    auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
        .AddMetadata(pitaya::constants::kGrpcHostKey, "localhost")
        .AddMetadata(pitaya::constants::kGrpcPortKey, "3030");

    _client->ServerAdded(server);

    protos::Request req;
    auto res = _client->Call(server, req);
    ASSERT_TRUE(res.has_error());
    EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
    EXPECT_TRUE(std::regex_search(res.error().msg(), std::regex("Call RPC failed")));
}
