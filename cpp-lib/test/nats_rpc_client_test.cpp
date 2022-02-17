#include "pitaya/constants.h"
#include "pitaya/nats/rpc_client.h"

#include "mock_nats_client.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace testing;
namespace constants = pitaya::constants;

class NatsRpcClientTest : public testing::Test
{
public:
    void SetUp() override
    {
        _mockNatsClient = new MockNatsClient();
        _config = pitaya::NatsConfig();
        _rpcClient = std::unique_ptr<pitaya::NatsRpcClient>(new pitaya::NatsRpcClient(
            _config, std::unique_ptr<pitaya::NatsClient>(_mockNatsClient)));
    }

    void TearDown() override { _rpcClient.reset(); }

protected:
    MockNatsClient* _mockNatsClient;
    pitaya::NatsConfig _config;
    std::unique_ptr<pitaya::NatsRpcClient> _rpcClient;
};

TEST_F(NatsRpcClientTest, CanBeCreatedAndDestroyed) {}

TEST_F(NatsRpcClientTest, CanSendRpcs)
{
    using namespace pitaya;

    auto mockNatsMsg = new MockNatsMsg();
    auto retMsg = std::shared_ptr<NatsMsg>(mockNatsMsg);

    EXPECT_CALL(*_mockNatsClient, Request(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<0>(retMsg), Return(NATS_OK)));

    // A success response will be returned
    protos::Response natsResData;
    natsResData.set_data("my awesome response data");

    std::vector<uint8_t> buffer(natsResData.ByteSizeLong());
    natsResData.SerializeToArray(buffer.data(), buffer.size());

    EXPECT_CALL(*mockNatsMsg, GetData()).WillOnce(Return(buffer.data()));

    EXPECT_CALL(*mockNatsMsg, GetSize()).WillOnce(Return(buffer.size()));

    auto target = pitaya::Server(pitaya::Server::Kind::Backend, "my-type", "my-id");
    protos::Request req;
    auto rpcRes = _rpcClient->Call(target, req);

    ASSERT_FALSE(rpcRes.has_error());
    EXPECT_EQ(rpcRes.data(), natsResData.data());
}

TEST_F(NatsRpcClientTest, RpcsCanReturnError)
{
    using namespace pitaya;

    auto mockNatsMsg = new MockNatsMsg();
    auto retMsg = std::shared_ptr<NatsMsg>(mockNatsMsg);

    EXPECT_CALL(*_mockNatsClient, Request(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<0>(retMsg), Return(NATS_OK)));

    auto error = new protos::Error();
    error->set_code("my-random-error-code");
    error->set_msg("my-random-error-msg");

    // An error response will be returned
    protos::Response natsResData;
    natsResData.set_allocated_error(error);

    std::vector<uint8_t> buffer(natsResData.ByteSizeLong());
    natsResData.SerializeToArray(buffer.data(), buffer.size());

    EXPECT_CALL(*mockNatsMsg, GetData()).WillOnce(Return(buffer.data()));

    EXPECT_CALL(*mockNatsMsg, GetSize()).WillOnce(Return(buffer.size()));

    auto target = pitaya::Server(pitaya::Server::Kind::Backend, "my-type", "my-id");
    protos::Request req;
    auto rpcRes = _rpcClient->Call(target, req);

    ASSERT_TRUE(rpcRes.has_error());
    EXPECT_EQ(rpcRes.error().code(), error->code());
    EXPECT_EQ(rpcRes.error().msg(), error->msg());
}

TEST_F(NatsRpcClientTest, RpcsCanFail)
{
    using namespace pitaya;

    auto mockNatsMsg = new MockNatsMsg();
    auto retMsg = std::shared_ptr<NatsMsg>(mockNatsMsg);

    EXPECT_CALL(*_mockNatsClient, Request(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<0>(retMsg), Return(NATS_ERR)));

    auto target = pitaya::Server(pitaya::Server::Kind::Backend, "my-type", "my-id");
    protos::Request req;
    auto rpcRes = _rpcClient->Call(target, req);

    ASSERT_TRUE(rpcRes.has_error());
    EXPECT_EQ(rpcRes.error().code(), constants::kCodeInternalError);
    EXPECT_EQ(rpcRes.error().msg(), "nats error - Error");
}

TEST_F(NatsRpcClientTest, CanSendKicks)
{
    using namespace pitaya;

    EXPECT_CALL(*_mockNatsClient, Request(_, _, _, _)).WillOnce(Return(NATS_OK));

    auto target = pitaya::Server(pitaya::Server::Kind::Backend, "my-type", "my-id");

    protos::KickMsg kick;
    kick.set_userid("my-user-id");

    auto error = _rpcClient->SendKickToUser(target.Id(), target.Type(), kick);
    ASSERT_FALSE(error);
}

TEST_F(NatsRpcClientTest, KicksCanFail)
{
    using namespace pitaya;

    EXPECT_CALL(*_mockNatsClient, Request(_, _, _, _)).WillOnce(Return(NATS_ERR));

    auto target = pitaya::Server(pitaya::Server::Kind::Backend, "my-type", "my-id");

    protos::KickMsg kick;
    kick.set_userid("my-user-id");

    auto error = _rpcClient->SendKickToUser(target.Id(), target.Type(), kick);
    ASSERT_TRUE(error);
    ASSERT_EQ(error->code, constants::kCodeInternalError);
    ASSERT_EQ(error->msg, "nats error - Error");
}

TEST_F(NatsRpcClientTest, KicksCanFailWithTimeout)
{
    using namespace pitaya;

    EXPECT_CALL(*_mockNatsClient, Request(_, _, _, _)).WillOnce(Return(NATS_TIMEOUT));

    auto target = pitaya::Server(pitaya::Server::Kind::Backend, "my-type", "my-id");

    protos::KickMsg kick;
    kick.set_userid("my-user-id");

    auto error = _rpcClient->SendKickToUser(target.Id(), target.Type(), kick);
    ASSERT_TRUE(error);
    ASSERT_EQ(error->code, constants::kCodeTimeout);
    ASSERT_EQ(error->msg, "nats timeout");
}

TEST_F(NatsRpcClientTest, CanSendUserPushes)
{
    using namespace pitaya;

    protos::Push push;
    push.set_uid("user-id");
    push.set_route("my.push.route");
    push.set_data("Push data");

    std::vector<uint8_t> pushData(push.ByteSizeLong());
    push.SerializeToArray(pushData.data(), pushData.size());

    EXPECT_CALL(*_mockNatsClient, Request(_, _, pushData, _config.requestTimeout))
        .WillOnce(Return(NATS_OK));

    auto target = pitaya::Server(pitaya::Server::Kind::Frontend, "my-type", "my-id");

    auto error = _rpcClient->SendPushToUser(target.Id(), target.Type(), push);
    ASSERT_FALSE(error);
}

TEST_F(NatsRpcClientTest, UserPushesCanFail)
{
    using namespace pitaya;

    protos::Push push;
    push.set_uid("user-id");
    push.set_route("my.push.route");
    push.set_data("Push data");

    std::vector<uint8_t> pushData(push.ByteSizeLong());
    push.SerializeToArray(pushData.data(), pushData.size());

    EXPECT_CALL(*_mockNatsClient, Request(_, _, pushData, _config.requestTimeout))
        .WillOnce(Return(NATS_ERR));

    auto target = pitaya::Server(pitaya::Server::Kind::Backend, "my-type", "my-id");

    auto error = _rpcClient->SendPushToUser(target.Id(), target.Type(), push);
    ASSERT_TRUE(error);
    EXPECT_EQ(error->code, constants::kCodeInternalError);
    EXPECT_EQ(error->msg, "nats error - Error");
}
