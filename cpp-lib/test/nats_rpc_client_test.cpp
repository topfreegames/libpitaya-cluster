#include "mock_nats_client.h"
#include "pitaya/nats/rpc_client.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace testing;

class NatsRpcClientTest : public testing::Test
{
public:
    void SetUp() override
    {
        _mockNatsClient = new MockNatsClient();
        _config = pitaya::NatsConfig();
        _rpcClient = std::unique_ptr<pitaya::NatsRpcClient>(new pitaya::NatsRpcClient(_config, std::unique_ptr<pitaya::NatsClient>(_mockNatsClient)));
    }

    void TearDown() override { _rpcClient.reset(); }

protected:
    MockNatsClient* _mockNatsClient;
    pitaya::NatsConfig _config;
    std::unique_ptr<pitaya::NatsRpcClient> _rpcClient;
};

TEST_F(NatsRpcClientTest, CanBeCreatedAndDestroyed)
{
}

//protos::Response
//NatsRpcClient::Call(const pitaya::Server& target, const protos::Request& req)
//{
//    auto topic = utils::GetTopicForServer(target.Id(), target.Type());
//
//    std::vector<uint8_t> buffer(req.ByteSizeLong());
//    req.SerializeToArray(buffer.data(), buffer.size());
//
//    std::shared_ptr<NatsMsg> reply;
//    NatsStatus status = _natsClient->Request(&reply, topic, buffer, _requestTimeout);
//
//    protos::Response res;
//
//    if (status != NatsStatus::Ok) {
//        auto err = new protos::Error();
//        if (status == NatsStatus::Timeout) {
//            err->set_code(constants::kCodeTimeout);
//            err->set_msg("nats timeout");
//        } else {
//            err->set_code(constants::kCodeInternalError);
//            err->set_msg("nats error");
//        }
//        res.set_allocated_error(err);
//    } else {
//        res.ParseFromArray(reply->GetData(), reply->GetSize());
//    }
//
//    return res;
//})

TEST_F(NatsRpcClientTest, CanSendRpcs)
{
    using namespace pitaya;

    auto mockNatsMsg = new MockNatsMsg();
    auto retMsg = std::shared_ptr<NatsMsg>(mockNatsMsg);

    EXPECT_CALL(*_mockNatsClient, Request(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<0>(retMsg), Return(NatsStatus::Ok)));

    // A success response will be returned
    protos::Response natsResData;
    natsResData.set_data("my awesome response data");

    std::vector<uint8_t> buffer(natsResData.ByteSizeLong());
    natsResData.SerializeToArray(buffer.data(), buffer.size());

    EXPECT_CALL(*mockNatsMsg, GetData())
      .WillOnce(Return(buffer.data()));

    EXPECT_CALL(*mockNatsMsg, GetSize())
      .WillOnce(Return(buffer.size()));

    auto target = pitaya::Server(pitaya::Server::Kind::Backend, "my-type", "my-id");
    protos::Request req;
    auto rpcRes = _rpcClient->Call(target, req);

    ASSERT_FALSE(rpcRes.has_error());
    EXPECT_EQ(rpcRes.data(), natsResData.data());
}
