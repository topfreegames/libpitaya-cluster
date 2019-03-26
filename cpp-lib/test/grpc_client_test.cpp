#include "test_common.h"

#include "pitaya.h"
#include "pitaya/constants.h"
#include "pitaya/grpc/rpc_client.h"
#include "pitaya/protos/pitaya_mock.grpc.pb.h"

#include "mock_etcd_client.h"
#include "mock_service_discovery.h"
#include <cpprest/json.h>
#include <regex>

namespace json = web::json;
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
        _config.connectionTimeout = std::chrono::seconds(1);
        _config.host = "localhost";
        _config.port = 3030;

        _numClientsUsed = 0;

        // NOTE: since the argument is `this`, we cannot know what `this` points to
        // before creating the client object, therefore this expectation is sub-optimal.
        EXPECT_CALL(*_mockSd, AddListener(_));
        EXPECT_CALL(*_mockSd, RemoveListener(_));

        _client = std::unique_ptr<pitaya::GrpcClient>(
            new pitaya::GrpcClient(_config,
                                   _sd,
                                   [this](std::shared_ptr<grpc::ChannelInterface> channel)
                                       -> std::unique_ptr<protos::Pitaya::StubInterface> {
                                       (void)channel;
                                       EXPECT_LE(_numClientsUsed + 1, _mockStubs.size());
                                       return std::unique_ptr<protos::Pitaya::StubInterface>(
                                           _mockStubs[_numClientsUsed++]);
                                   }));
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
    std::vector<protos::Pitaya::StubInterface*> _mockStubs;
    std::unique_ptr<pitaya::GrpcClient> _client;
    size_t _numClientsUsed;
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
    {
        pitaya::Server server(pitaya::Server::Kind::Frontend, "server-id", "server-type");
        _client->ServerAdded(server);

        protos::Request req;

        auto res = _client->Call(server, req);
        ASSERT_TRUE(res.has_error());
        EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
        EXPECT_TRUE(std::regex_search(res.error().msg(),
                                      std::regex("is not added to the connections map")));
    }
    {
        auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
                          .WithMetadata(pitaya::constants::kGrpcHostKey, "host");

        _client->ServerAdded(server);

        protos::Request req;
        auto res = _client->Call(server, req);
        ASSERT_TRUE(res.has_error());
        EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
        EXPECT_TRUE(std::regex_search(res.error().msg(),
                                      std::regex("is not added to the connections map")));
    }
    {
        auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
                          .WithMetadata(pitaya::constants::kGrpcPortKey, "3030");

        _client->ServerAdded(server);

        protos::Request req;
        auto res = _client->Call(server, req);
        ASSERT_TRUE(res.has_error());
        EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
        EXPECT_TRUE(std::regex_search(res.error().msg(),
                                      std::regex("is not added to the connections map")));
    }
}

TEST_F(GrpcClientTest, RemovingUnexistentServersIsIgnored)
{
    pitaya::Server server(pitaya::Server::Kind::Frontend, "server-id", "server-type");
    _client->ServerRemoved(server);
}

TEST_F(GrpcClientTest, RpcsCanFail)
{
    auto mockStub = new protos::MockPitayaStub();
    _mockStubs.push_back(mockStub);

    auto msg = new protos::Msg();
    msg->set_data("this is my custom data");

    protos::Request req;
    req.set_type(protos::RPCType::User);
    req.set_allocated_msg(msg);

    auto msgMatcher =
        Property(&protos::Request::msg, Property(&protos::Msg::data, "this is my custom data"));
    auto reqMatcher = AllOf(Property(&protos::Request::type, protos::RPCType::User), msgMatcher);

    EXPECT_CALL(*mockStub, Call(_, reqMatcher, _)).WillOnce(Return(grpc::Status::CANCELLED));

    auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
                      .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
                      .WithMetadata(pitaya::constants::kGrpcPortKey, "3030");

    _client->ServerAdded(server);

    auto res = _client->Call(server, req);
    ASSERT_TRUE(res.has_error());
    EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
    EXPECT_TRUE(std::regex_search(res.error().msg(), std::regex("Call RPC failed")));

    _client->ServerRemoved(server);
}

TEST_F(GrpcClientTest, CanSuccessfullyDoRpc)
{
    auto mockStub = new protos::MockPitayaStub();
    _mockStubs.push_back(mockStub);

    protos::Response resResult;
    resResult.set_data("return data");

    auto msgMatcher =
        Property(&protos::Request::msg, Property(&protos::Msg::data, "my special data"));
    auto reqMatcher = AllOf(Property(&protos::Request::type, protos::RPCType::User), msgMatcher);
    EXPECT_CALL(*mockStub, Call(_, reqMatcher, _))
        .WillOnce(DoAll(SetArgPointee<2>(resResult), Return(grpc::Status::OK)));

    auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
                      .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
                      .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    _client->ServerAdded(server);

    auto msg = new protos::Msg();
    msg->set_type(protos::MsgType::MsgRequest);
    msg->set_route("room.room.testremote");
    msg->set_data("my special data");

    protos::Request req;
    req.set_allocated_msg(msg);
    req.set_metadata("{}");
    req.set_type(protos::RPCType::User);

    auto res = _client->Call(server, req);
    ASSERT_FALSE(res.has_error());
    EXPECT_EQ(res.data(), resResult.data());
}
