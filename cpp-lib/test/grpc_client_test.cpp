#include "test_common.h"

#include "pitaya.h"
#include "pitaya/constants.h"
#include "pitaya/grpc/rpc_client.h"
#include "pitaya/grpc/rpc_server.h"
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
        _config.host = "localhost";
        _config.port = 3030;

        _numClientsUsed = 0;

        // NOTE: since the argument is `this`, we cannot know what `this` points to
        // before creating the client object, therefore this expectation is sub-optimal.
        EXPECT_CALL(*_mockSd, AddListener(_));
        EXPECT_CALL(*_mockSd, RemoveListener(_));
    }

    std::unique_ptr<pitaya::GrpcClient> CreateClient(
        std::function<std::unique_ptr<protos::Pitaya::StubInterface>(std::shared_ptr<grpc::ChannelInterface>)> providedStubCreator = nullptr)
    {
        std::function<std::unique_ptr<protos::Pitaya::StubInterface>(std::shared_ptr<grpc::ChannelInterface>)> stubCreator;
        if (providedStubCreator) {
            stubCreator = providedStubCreator;
        } else {
            stubCreator = [this](std::shared_ptr<grpc::ChannelInterface> channel) -> std::unique_ptr<protos::Pitaya::StubInterface> {
                (void)channel;
                EXPECT_LE(_numClientsUsed + 1, _mockStubs.size());
                return std::unique_ptr<protos::Pitaya::StubInterface>(
                    _mockStubs[_numClientsUsed++]);
            };
        }
        return std::unique_ptr<pitaya::GrpcClient>(
            new pitaya::GrpcClient(_config,
                                   _sd,
                                   std::unique_ptr<pitaya::BindingStorage>(_mockBs),
                                   stubCreator));
    }

    std::unique_ptr<pitaya::GrpcServer> CreateServer(pitaya::RpcHandlerFunc handler)
    {
        auto server = std::unique_ptr<pitaya::GrpcServer>(new pitaya::GrpcServer(_config));
        server->Start(std::move(handler));
        return server;
    }

    void TearDown() override
    {
        _sd.reset();
    }

protected:
    MockServiceDiscovery* _mockSd;
    MockBindingStorage* _mockBs;
    std::shared_ptr<ServiceDiscovery> _sd;
    pitaya::GrpcConfig _config;
    std::vector<protos::Pitaya::StubInterface*> _mockStubs;
    size_t _numClientsUsed;
};

TEST_F(GrpcClientTest, CallFailsWhenNoConnectionsExist)
{
    pitaya::Server target(pitaya::Server::Kind::Frontend, "myid", "mytype");
    protos::Request req;

    auto client = CreateClient();
    auto res = client->Call(target, req);
    ASSERT_TRUE(res.has_error());
    EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
    EXPECT_FALSE(res.error().msg().empty());
}

TEST_F(GrpcClientTest, ServersWithoutGrpcSupportAreIgnored)
{
    auto client = CreateClient();
    {
        pitaya::Server server(pitaya::Server::Kind::Frontend, "server-id", "server-type");

        client->ServerAdded(server);

        protos::Request req;

        auto res = client->Call(server, req);
        ASSERT_TRUE(res.has_error());
        EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
        EXPECT_TRUE(std::regex_search(res.error().msg(),
                                      std::regex("is not added to the connections map")));
    }
    {
        auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
                          .WithMetadata(pitaya::constants::kGrpcHostKey, "host");

        client->ServerAdded(server);

        protos::Request req;
        auto res = client->Call(server, req);
        ASSERT_TRUE(res.has_error());
        EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
        EXPECT_TRUE(std::regex_search(res.error().msg(),
                                      std::regex("is not added to the connections map")));
    }
    {
        auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
                          .WithMetadata(pitaya::constants::kGrpcPortKey, "3030");

        client->ServerAdded(server);

        protos::Request req;
        auto res = client->Call(server, req);
        ASSERT_TRUE(res.has_error());
        EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
        EXPECT_TRUE(std::regex_search(res.error().msg(),
                                      std::regex("is not added to the connections map")));
    }
}

TEST_F(GrpcClientTest, RemovingUnexistentServersIsIgnored)
{
    auto client = CreateClient();
    pitaya::Server server(pitaya::Server::Kind::Frontend, "server-id", "server-type");
    client->ServerRemoved(server);
}

TEST_F(GrpcClientTest, RpcsCanFail)
{
    auto client = CreateClient();
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

    client->ServerAdded(server);

    auto res = client->Call(server, req);
    ASSERT_TRUE(res.has_error());
    EXPECT_EQ(res.error().code(), pitaya::constants::kCodeInternalError);
    EXPECT_TRUE(std::regex_search(res.error().msg(), std::regex("Call RPC failed")));

    client->ServerRemoved(server);
}

TEST_F(GrpcClientTest, CanSuccessfullyDoRpc)
{
    auto client = CreateClient();
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

    client->ServerAdded(server);

    auto msg = new protos::Msg();
    msg->set_type(protos::MsgType::MsgRequest);
    msg->set_route("room.room.testremote");
    msg->set_data("my special data");

    protos::Request req;
    req.set_allocated_msg(msg);
    req.set_metadata("{}");
    req.set_type(protos::RPCType::User);

    auto res = client->Call(server, req);
    ASSERT_FALSE(res.has_error());
    EXPECT_EQ(res.data(), resResult.data());
}

TEST_F(GrpcClientTest, CanSuccessfullySendPushesWithId)
{
    auto client = CreateClient();
    auto mockStub = new protos::MockPitayaStub();
    _mockStubs.push_back(mockStub);

    auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
                      .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
                      .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    client->ServerAdded(server);

    auto reqMatcher = AllOf(Property(&protos::Push::uid, "user-uid"),
                            Property(&protos::Push::data, "MY PUSH DATA"),
                            Property(&protos::Push::route, "my.push.route"));

    EXPECT_CALL(*mockStub, PushToUser(_, reqMatcher, _)).WillOnce(Return(grpc::Status::OK));

    protos::Push push;
    push.set_uid("user-uid");
    push.set_data("MY PUSH DATA");
    push.set_route("my.push.route");

    auto res = client->SendPushToUser("server-id", "server-type", push);
    ASSERT_FALSE(res);
}

TEST_F(GrpcClientTest, CanSuccessfullySendPushesWithoutId)
{
    auto client = CreateClient();
    auto mockStub1 = new protos::MockPitayaStub();
    auto mockStub2 = new protos::MockPitayaStub();
    _mockStubs.push_back(mockStub1);
    _mockStubs.push_back(mockStub2);

    auto server1 = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
                       .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
                       .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    auto server2 =
        pitaya::Server(pitaya::Server::Kind::Frontend, "my-special-server-id", "server-type")
            .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
            .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    client->ServerAdded(server1);
    client->ServerAdded(server2);

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

        EXPECT_CALL(*mockStub2, PushToUser(_, reqMatcher, _)).WillOnce(Return(grpc::Status::OK));
    }

    auto res = client->SendPushToUser("", "server-type", push);
    ASSERT_FALSE(res);
}

TEST_F(GrpcClientTest, SendPushFailsIfServerIdIsNotFoundForUid)
{
    auto client = CreateClient();
    protos::Response resResult;
    resResult.set_data("return data");

    protos::Push push;
    push.set_uid("user-uid");
    push.set_data("MY PUSH DATA");
    push.set_route("my.push.route");

    InSequence seq;
    EXPECT_CALL(*_mockBs, GetUserFrontendId(push.uid(), "server-type"))
        .WillOnce(Throw(pitaya::PitayaException("Custom Exception Message")));

    auto res = client->SendPushToUser("", "server-type", push);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->msg, "Custom Exception Message");
}

TEST_F(GrpcClientTest, SendPushFailsIfThereAreNoServersOnTheConnectionMap)
{
    auto client = CreateClient();
    protos::Response resResult;
    resResult.set_data("return data");

    protos::Push push;
    push.set_uid("user-uid");
    push.set_data("MY PUSH DATA");
    push.set_route("my.push.route");

    InSequence seq;
    EXPECT_CALL(*_mockBs, GetUserFrontendId(push.uid(), "server-type"))
        .WillOnce(Return("my-server-id"));

    auto res = client->SendPushToUser("", "server-type", push);
    ASSERT_TRUE(res);
    EXPECT_TRUE(std::regex_search(res->msg, std::regex("not added to the connections map")));
}

TEST_F(GrpcClientTest, SendPushFailsIfGrpcCallFails)
{
    auto client = CreateClient();
    protos::Push push;
    push.set_uid("user-uid");
    push.set_data("MY PUSH DATA");
    push.set_route("my.push.route");

    auto mockStub = new protos::MockPitayaStub();
    _mockStubs.push_back(mockStub);

    auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "my-server-id", "server-type")
                      .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
                      .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    client->ServerAdded(server);

    protos::Response resResult;
    resResult.set_data("return data");

    EXPECT_CALL(*mockStub, PushToUser(_, _, _)).WillOnce(Return(grpc::Status::CANCELLED));

    auto res = client->SendPushToUser("my-server-id", "server-type", push);
    ASSERT_TRUE(res);
    EXPECT_TRUE(std::regex_search(res->msg, std::regex("Push failed:")));
}

TEST_F(GrpcClientTest, CanSuccessfullySendKickWithId)
{
    auto client = CreateClient();
    auto mockStub = new protos::MockPitayaStub();
    _mockStubs.push_back(mockStub);

    protos::Response resResult;
    resResult.set_data("return data");

    auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
                      .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
                      .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    client->ServerAdded(server);

    protos::KickMsg kick;
    kick.set_userid("user-uid");

    EXPECT_CALL(*mockStub, KickUser(_, Property(&protos::KickMsg::userid, kick.userid()), _))
        .WillOnce(Return(grpc::Status::OK));

    auto error = client->SendKickToUser(server.Id(), server.Type(), kick);
    ASSERT_FALSE(error);
}

TEST_F(GrpcClientTest, CanSuccessfullySendKickWithoutId)
{
    auto client = CreateClient();
    auto mockStub1 = new protos::MockPitayaStub();
    auto mockStub2 = new protos::MockPitayaStub();
    _mockStubs.push_back(mockStub1);
    _mockStubs.push_back(mockStub2);

    auto server1 = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
                       .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
                       .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    auto server2 =
        pitaya::Server(pitaya::Server::Kind::Frontend, "my-special-server-id", "server-type")
            .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
            .WithMetadata(pitaya::constants::kGrpcPortKey, "3435");

    client->ServerAdded(server1);
    client->ServerAdded(server2);

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
        auto error = client->SendKickToUser("", "server-type", kick1);
        EXPECT_FALSE(error);
    }

    {
        auto error = client->SendKickToUser("", "server-type", kick2);
        EXPECT_FALSE(error);
    }
}

TEST_F(GrpcClientTest, RpcsCanTimeout)
{
    using std::chrono::milliseconds;
    // Set one second of timeout
    _config.clientRpcTimeout = milliseconds(1000);

    bool called = false;

    // Create the server
    auto rpcServer = CreateServer([&](const protos::Request& req, pitaya::Rpc* rpc) {
        if (rpc) {
            // Wait more than one second to response in order to trigger a timeout
            std::this_thread::sleep_for(_config.clientRpcTimeout + milliseconds(50));
            EXPECT_EQ(req.msg().route(), "my.custom.route");
            EXPECT_EQ(req.msg().data(), "my special data");

            protos::Response res;
            res.set_data("SERVER DATA");
            called = true;
            rpc->Finish(res);
        }
    });

    auto client = CreateClient([](std::shared_ptr<grpc::ChannelInterface> channel) -> std::unique_ptr<protos::Pitaya::StubInterface> {
        return protos::Pitaya::NewStub(channel);
    });

    protos::Response resResult;
    resResult.set_data("return data");

    auto server = pitaya::Server(pitaya::Server::Kind::Frontend, "server-id", "server-type")
                      .WithMetadata(pitaya::constants::kGrpcHostKey, "localhost")
                      .WithMetadata(pitaya::constants::kGrpcPortKey, "3030");

    client->ServerAdded(server);

    auto msg = new protos::Msg();
    msg->set_type(protos::MsgType::MsgRequest);
    msg->set_route("my.custom.route");
    msg->set_data("my special data");

    protos::Request req;
    req.set_allocated_msg(msg);
    req.set_metadata("{}");
    req.set_type(protos::RPCType::User);

    auto res = client->Call(server, req);
    ASSERT_TRUE(res.has_error());
    EXPECT_EQ(res.error().code(), pitaya::constants::kCodeTimeout);
    
    rpcServer->Shutdown();
    
    EXPECT_TRUE(called);
}
