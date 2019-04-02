#include "test_common.h"

#include "pitaya.h"
#include "pitaya/constants.h"
#include "pitaya/grpc/rpc_client.h"
#include "pitaya/protos/pitaya_mock.grpc.pb.h"

#include "mock_binding_storage.h"
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
        _mockBs = new MockBindingStorage();
        _sd = std::shared_ptr<ServiceDiscovery>(_mockSd);

        _config = pitaya::GrpcConfig();
        _config.connectionTimeout = std::chrono::milliseconds(100);
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
                                   std::unique_ptr<pitaya::BindingStorage>(_mockBs),
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
    MockBindingStorage* _mockBs;
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

TEST_F(GrpcClientTest, CanSuccessfullySendPushesWithId)
{
    auto mockStub = new protos::MockPitayaStub();
    _mockStubs.push_back(mockStub);

    auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
                      .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
                      .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    _client->ServerAdded(server);

    auto reqMatcher = AllOf(Property(&protos::Push::uid, "user-uid"),
                            Property(&protos::Push::data, "MY PUSH DATA"),
                            Property(&protos::Push::route, "my.push.route"));

    EXPECT_CALL(*mockStub, PushToUser(_, reqMatcher, _))
        .WillOnce(Return(grpc::Status::OK));

    protos::Push push;
    push.set_uid("user-uid");
    push.set_data("MY PUSH DATA");
    push.set_route("my.push.route");

    auto res = _client->SendPushToUser("server-id", "server-type", push);
    ASSERT_FALSE(res);
}

TEST_F(GrpcClientTest, CanSuccessfullySendPushesWithoutId)
{
    auto mockStub1 = new protos::MockPitayaStub();
    auto mockStub2 = new protos::MockPitayaStub();
    _mockStubs.push_back(mockStub1);
    _mockStubs.push_back(mockStub2);

    auto server1 = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
        .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
        .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    auto server2 = pitaya::Server(pitaya::Server::Kind::Frontend, "my-special-server-id", "server-type")
        .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
        .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    _client->ServerAdded(server1);
    _client->ServerAdded(server2);

    protos::Push push;
    push.set_uid("user-uid");
    push.set_data("MY PUSH DATA");
    push.set_route("my.push.route");

    {
        InSequence seq;
        EXPECT_CALL(*_mockBs, GetUserFrontendId(push.uid(), "server-type"))
            .WillOnce(Return(server2.Id()));

        auto reqMatcher = AllOf(Property(&protos::Push::uid, "user-uid"),
                                Property(&protos::Push::data, push.data()),
                                Property(&protos::Push::route, push.route()));

        EXPECT_CALL(*mockStub2, PushToUser(_, reqMatcher, _))
            .WillOnce(Return(grpc::Status::OK));
    }

    auto res = _client->SendPushToUser("", "server-type", push);
    ASSERT_FALSE(res);
}

TEST_F(GrpcClientTest, SendPushFailsIfServerIdIsNotFoundForUid)
{
    protos::Response resResult;
    resResult.set_data("return data");

    protos::Push push;
    push.set_uid("user-uid");
    push.set_data("MY PUSH DATA");
    push.set_route("my.push.route");

    InSequence seq;
    EXPECT_CALL(*_mockBs, GetUserFrontendId(push.uid(), "server-type"))
        .WillOnce(Throw(pitaya::PitayaException("Custom Exception Message")));

    auto res = _client->SendPushToUser("", "server-type", push);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->msg, "Custom Exception Message");
}

TEST_F(GrpcClientTest, SendPushFailsIfThereAreNoServersOnTheConnectionMap)
{
    protos::Response resResult;
    resResult.set_data("return data");

    protos::Push push;
    push.set_uid("user-uid");
    push.set_data("MY PUSH DATA");
    push.set_route("my.push.route");

    InSequence seq;
    EXPECT_CALL(*_mockBs, GetUserFrontendId(push.uid(), "server-type"))
        .WillOnce(Return("my-server-id"));

    auto res = _client->SendPushToUser("", "server-type", push);
    ASSERT_TRUE(res);
    EXPECT_TRUE(std::regex_search(res->msg, std::regex("not added to the connections map")));
}

TEST_F(GrpcClientTest, SendPushFailsIfGrpcCallFails)
{
    protos::Push push;
    push.set_uid("user-uid");
    push.set_data("MY PUSH DATA");
    push.set_route("my.push.route");

    auto mockStub = new protos::MockPitayaStub();
    _mockStubs.push_back(mockStub);

    auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "my-server-id", "server-type")
        .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
        .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    _client->ServerAdded(server);

    protos::Response resResult;
    resResult.set_data("return data");

    EXPECT_CALL(*mockStub, PushToUser(_, _, _))
        .WillOnce(Return(grpc::Status::CANCELLED));

    auto res = _client->SendPushToUser("my-server-id", "server-type", push);
    ASSERT_TRUE(res);
    EXPECT_TRUE(std::regex_search(res->msg, std::regex("Push failed:")));
}

TEST_F(GrpcClientTest, CanSuccessfullySendKickWithId)
{
    auto mockStub = new protos::MockPitayaStub();
    _mockStubs.push_back(mockStub);

    protos::Response resResult;
    resResult.set_data("return data");

    auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
        .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
        .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    _client->ServerAdded(server);

    protos::KickMsg kick;
    kick.set_userid("user-uid");

    EXPECT_CALL(*mockStub, KickUser(_, Property(&protos::KickMsg::userid, kick.userid()), _))
        .WillOnce(Return(grpc::Status::OK));

    auto error = _client->SendKickToUser(server.Id(), server.Type(), kick);
    ASSERT_FALSE(error);
}

TEST_F(GrpcClientTest, CanSuccessfullySendKickWithoutId)
{
    auto mockStub1 = new protos::MockPitayaStub();
    auto mockStub2 = new protos::MockPitayaStub();
    _mockStubs.push_back(mockStub1);
    _mockStubs.push_back(mockStub2);

    auto server1 = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
        .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
        .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    auto server2 = pitaya::Server(pitaya::Server::Kind::Frontend, "my-special-server-id", "server-type")
        .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
        .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    _client->ServerAdded(server1);
    _client->ServerAdded(server2);

    protos::KickMsg kick1;
    kick1.set_userid("user-uid-1");

    protos::KickMsg kick2;
    kick2.set_userid("user-uid-2");

    {
        InSequence seq;
        EXPECT_CALL(*_mockBs, GetUserFrontendId(kick1.userid(), "server-type"))
            .WillOnce(Return(server2.Id()));
        EXPECT_CALL(*mockStub2, KickUser(_, Property(&protos::KickMsg::userid, kick1.userid()), _))
            .WillOnce(Return(grpc::Status::OK));

        EXPECT_CALL(*_mockBs, GetUserFrontendId(kick2.userid(), "server-type"))
            .WillOnce(Return(server1.Id()));
        EXPECT_CALL(*mockStub1, KickUser(_, Property(&protos::KickMsg::userid, kick2.userid()), _))
            .WillOnce(Return(grpc::Status::OK));
    }

    {
        auto error = _client->SendKickToUser("", "server-type", kick1);
        EXPECT_FALSE(error);
    }

    {
        auto error = _client->SendKickToUser("", "server-type", kick2);
        EXPECT_FALSE(error);
    }
}

