#include "test_common.h"

#include "pitaya/cluster.h"
#include "pitaya/constants.h"
#include "pitaya/grpc/rpc_server.h"

#include "mock_rpc_client.h"
#include "mock_rpc_server.h"
#include "mock_service_discovery.h"
#include <boost/optional.hpp>
#include <memory>

using namespace pitaya;
using namespace ::testing;
using boost::optional;
using pitaya::service_discovery::ServiceDiscovery;

class ClusterTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        _mockSd = new MockServiceDiscovery();
        _mockRpcSv = new MockRpcServer();
        _mockRpcClient = new MockRpcClient();

        _server = Server(Server::Kind::Backend, "my-server-id", "connector");

        EXPECT_CALL(*_mockRpcSv, Start(_)).WillOnce(SaveArg<0>(&_handlerFunc));

        pitaya::Cluster::Instance().Initialize(_server,
                                               std::shared_ptr<ServiceDiscovery>(_mockSd),
                                               std::unique_ptr<RpcServer>(_mockRpcSv),
                                               std::unique_ptr<RpcClient>(_mockRpcClient));
    }

    void TearDown() override { pitaya::Cluster::Instance().Terminate(); }

protected:
    Server _server;
    MockServiceDiscovery* _mockSd;
    MockRpcServer* _mockRpcSv;
    MockRpcClient* _mockRpcClient;
    pitaya::RpcHandlerFunc _handlerFunc;
};

TEST_F(ClusterTest, RpcsCanBeDoneSuccessfuly)
{
    EXPECT_CALL(*_mockRpcSv, Shutdown());
    Server serverToReturn(Server::Kind::Backend, "my-server-id", "connector", "random-host");

    protos::Response resToReturn;
    resToReturn.set_data("ABACATE");

    {
        // InSequence seq;
        EXPECT_CALL(*_mockSd, GetServerById("my-server-id")).WillOnce(Return(serverToReturn));

        EXPECT_CALL(
            *_mockRpcClient,
            Call(Eq(serverToReturn), Property(&protos::Request::type, Eq(protos::RPCType::User))))
            .WillOnce(Return(resToReturn));
    }

    auto msg = new protos::Msg();
    msg->set_data("hello my friend");
    msg->set_route("sometype.handler.method");

    protos::Request req;
    req.set_allocated_msg(msg);
    req.set_type(protos::RPCType::User);

    protos::Response res;

    {
        optional<PitayaError> err =
            Cluster::Instance().RPC("my-server-id", "mytest.route", req, res);
        EXPECT_FALSE(err);
    }

    {
        // InSequence seq;

        EXPECT_CALL(*_mockSd, GetServersByType("mytest"))
            .WillOnce(Return(std::vector<pitaya::Server>{ serverToReturn }));

        EXPECT_CALL(*_mockSd, GetServerById("my-server-id")).WillOnce(Return(serverToReturn));

        EXPECT_CALL(
            *_mockRpcClient,
            Call(Eq(serverToReturn), Property(&protos::Request::type, Eq(protos::RPCType::User))))
            .WillOnce(Return(resToReturn));
    }

    {
        optional<PitayaError> err = Cluster::Instance().RPC("mytest.routehandler.route", req, res);
        EXPECT_FALSE(err);
    }
}

TEST_F(ClusterTest, FailsWhenNoServerIsFound)
{
    EXPECT_CALL(*_mockSd, GetServerById("my-server-id")).WillOnce(Return(boost::none));
    EXPECT_CALL(*_mockRpcSv, Shutdown());

    auto msg = new protos::Msg();
    msg->set_data("hello my friend");
    msg->set_route("someroute");

    protos::Request req;
    req.set_allocated_msg(msg);

    protos::Response res;

    optional<PitayaError> err = Cluster::Instance().RPC("my-server-id", "mytest.route", req, res);

    EXPECT_TRUE(err);

    PitayaError pErr = err.value();
    EXPECT_EQ(pErr.code, constants::kCodeNotFound);
}

TEST_F(ClusterTest, RpcReturnsErrorWhenTheCallFails)
{
    EXPECT_CALL(*_mockRpcSv, Shutdown());

    Server serverToReturn(Server::Kind::Backend, "my-server-id", "connector", "random-host");

    Sequence seq;

    EXPECT_CALL(*_mockSd, GetServerById(Eq("my-server-id")))
        .InSequence(seq)
        .WillOnce(Return(serverToReturn));

    protos::Response resToReturn;
    {
        auto error = new protos::Error();
        error->set_code(constants::kCodeInternalError);
        error->set_msg("Horrible error");
        resToReturn.set_allocated_error(error);
    }

    EXPECT_CALL(
        *_mockRpcClient,
        Call(Eq(serverToReturn), Property(&protos::Request::type, Eq(protos::RPCType::User))))
        .InSequence(seq)
        .WillOnce(Return(resToReturn));

    auto msg = new protos::Msg();
    msg->set_data("hello my friend");
    msg->set_route("someroute");

    protos::Request req;
    req.set_allocated_msg(msg);
    req.set_type(protos::RPCType::User);

    protos::Response res;

    optional<PitayaError> err = Cluster::Instance().RPC("my-server-id", "mytest.route", req, res);

    ASSERT_TRUE(err);

    PitayaError pErr = err.value();
    EXPECT_EQ(pErr.code, constants::kCodeInternalError);
    EXPECT_EQ(pErr.msg, "Horrible error");
}

ACTION_P(SendEmptyRpc, handlerFunc)
{
    handlerFunc(protos::Request(), nullptr);
}

TEST_F(ClusterTest, ThreadsWaitingForRpcsReceiveFinishedNotification)
{
    using namespace std::chrono;

    EXPECT_NE(_handlerFunc, nullptr);
    EXPECT_CALL(*_mockRpcSv, Shutdown()).WillOnce(SendEmptyRpc(_handlerFunc));

    // Define 10 threads waiting for RPCs
    std::vector<std::thread> rpcListeners(10);

    for (auto& thread : rpcListeners) {
        thread = std::thread([]() {
            optional<Cluster::RpcData> data = Cluster::Instance().WaitForRpc();
            EXPECT_EQ(data, boost::none);
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    Cluster::Instance().Terminate();

    for (auto& thread : rpcListeners) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
