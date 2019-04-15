#include "pitaya/constants.h"
#include "pitaya/nats/rpc_server.h"

#include "mock_nats_client.h"
#include "mock_rpc_server.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace pitaya;
namespace constants = pitaya::constants;

class NatsRpcServerTest : public testing::Test
{
public:
    void SetUp() override
    {
        _server = pitaya::Server(pitaya::Server::Kind::Backend, "my-id", "my-type");
        _config = NatsConfig();
        _config.natsAddr = "";
    }

    std::unique_ptr<NatsRpcServer> CreateServer(MockNatsClient* client)
    {
        auto server = std::unique_ptr<NatsRpcServer>(
            new pitaya::NatsRpcServer(_server, _config, std::unique_ptr<NatsClient>(client)));
        return server;
    }

protected:
    pitaya::Server _server;
    pitaya::NatsConfig _config;
};

TEST_F(NatsRpcServerTest, CanBeCreatedAndDestroyed)
{
    auto mockClient = new MockNatsClient();
    auto server = CreateServer(mockClient);
}

TEST_F(NatsRpcServerTest, CanBeStartedAndStopped)
{
    auto mockClient = new MockNatsClient();

    EXPECT_CALL(*mockClient, Subscribe(_, _))
        .WillOnce(Return((void*)0xdeadbeef)); // mock of valid pointer

    auto server = CreateServer(mockClient);
    bool called = false;
    server->Start([&called](const protos::Request& req, pitaya::Rpc* rpc) {
        EXPECT_EQ(rpc, nullptr);
        called = true;
    });
    server->Shutdown();

    EXPECT_TRUE(called);
}

TEST_F(NatsRpcServerTest, StartThrowsExceptionIfItFails)
{
    auto mockClient = new MockNatsClient();

    EXPECT_CALL(*mockClient, Subscribe(_, _)).WillOnce(Return(nullptr)); // mock of valid pointer

    auto server = CreateServer(mockClient);
    EXPECT_THROW(
        server->Start([](const protos::Request& req, pitaya::Rpc* rpc) { EXPECT_TRUE(false); }),
        PitayaException);
}

static std::thread gCallbackThread;

ACTION_TEMPLATE(ExecuteCallback, HAS_1_TEMPLATE_PARAMS(unsigned, Index), AND_1_VALUE_PARAMS(a))
{
    auto fn = std::get<Index>(args);

    if (gCallbackThread.joinable()) {
        gCallbackThread.join();
    }

    gCallbackThread = std::thread([=] { fn(a); });
}

TEST_F(NatsRpcServerTest, CanHandleRpcs)
{
    bool called = false;
    pitaya::Rpc* lastRpc = reinterpret_cast<pitaya::Rpc*>(0xdedbeef);
    auto mockClient = new MockNatsClient();

    // The response that the server will send
    protos::Response serverResponse;
    serverResponse.set_data("SERVER RESONSE DATA");

    auto mockNatsMsg = new MockNatsMsg();
    auto msg = new protos::Msg();
    msg->set_data("MY REQUEST DATA");

    protos::Request req;
    req.set_type(protos::RPCType::User);
    req.set_allocated_msg(msg);

    std::vector<uint8_t> buf(req.ByteSizeLong());
    req.SerializeToArray(buf.data(), buf.size());

    EXPECT_CALL(*mockNatsMsg, GetSize()).WillOnce(Return(buf.size()));
    EXPECT_CALL(*mockNatsMsg, GetData()).WillOnce(Return(buf.data()));
    EXPECT_CALL(*mockNatsMsg, GetReply()).WillOnce(Return("my.reply.server"));

    {
        void* returnedSubscriptionHandle = (void*)0xdeadbeef;

        EXPECT_CALL(*mockClient, Subscribe(_, _))
            .WillOnce(DoAll(ExecuteCallback<1>(std::shared_ptr<NatsMsg>(mockNatsMsg)),
                            Return(returnedSubscriptionHandle)));

        std::vector<uint8_t> serverResponseBuf(serverResponse.ByteSizeLong());
        serverResponse.SerializeToArray(serverResponseBuf.data(), serverResponseBuf.size());

        EXPECT_CALL(*mockClient, Publish("my.reply.server", serverResponseBuf))
            .WillOnce(Return(NatsStatus::Ok));
    }

    auto server = CreateServer(mockClient);

    server->Start([&](const protos::Request& req, pitaya::Rpc* rpc) {
        lastRpc = rpc;
        if (rpc) {
            called = true;
            rpc->Finish(serverResponse);
        }
    });

    if (gCallbackThread.joinable()) {
        gCallbackThread.join();
    }

    server->Shutdown();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_TRUE(called);
    EXPECT_EQ(lastRpc, nullptr);
}

TEST_F(NatsRpcServerTest, HasGracefulShutdown)
{
    using std::chrono::milliseconds;

    bool called = false;
    pitaya::Rpc* lastRpc = reinterpret_cast<pitaya::Rpc*>(0xdedbeef);
    auto mockClient = new MockNatsClient();

    // The response that the server will send
    protos::Response serverResponse;
    serverResponse.set_data("SERVER RESONSE DATA");

    auto mockNatsMsg = new MockNatsMsg();
    auto msg = new protos::Msg();
    msg->set_data("MY REQUEST DATA");

    protos::Request req;
    req.set_type(protos::RPCType::User);
    req.set_allocated_msg(msg);

    std::vector<uint8_t> buf(req.ByteSizeLong());
    req.SerializeToArray(buf.data(), buf.size());

    EXPECT_CALL(*mockNatsMsg, GetSize()).WillOnce(Return(buf.size()));
    EXPECT_CALL(*mockNatsMsg, GetData()).WillOnce(Return(buf.data()));
    EXPECT_CALL(*mockNatsMsg, GetReply()).WillOnce(Return("my.reply.server"));

    {
        void* returnedSubscriptionHandle = (void*)0xdeadbeef;
        EXPECT_CALL(*mockClient, Subscribe(_, _))
            .WillOnce(DoAll(ExecuteCallback<1>(std::shared_ptr<NatsMsg>(mockNatsMsg)),
                            Return(returnedSubscriptionHandle)));

        std::vector<uint8_t> serverResponseBuf(serverResponse.ByteSizeLong());
        serverResponse.SerializeToArray(serverResponseBuf.data(), serverResponseBuf.size());

        EXPECT_CALL(*mockClient, Publish("my.reply.server", serverResponseBuf))
            .WillOnce(Return(NatsStatus::Ok));
    }

    auto server = CreateServer(mockClient);

    server->Start([&](const protos::Request& req, pitaya::Rpc* rpc) {
        lastRpc = rpc;
        called = true;
        if (rpc) {
            auto t = std::thread([=]() {
                std::this_thread::sleep_for(milliseconds(100));
                EXPECT_NE(rpc, nullptr);
                rpc->Finish(serverResponse);
            });
            t.detach();
        }
    });

    std::this_thread::sleep_for(milliseconds(50));

    server->Shutdown();

    if (gCallbackThread.joinable()) {
        gCallbackThread.join();
    }
    
    std::this_thread::sleep_for(milliseconds(200));

    EXPECT_TRUE(called);
    EXPECT_EQ(lastRpc, nullptr);
}
