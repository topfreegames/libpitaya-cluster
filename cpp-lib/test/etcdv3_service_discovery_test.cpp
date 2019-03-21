#include "test_common.h"

#include "pitaya.h"
#include "pitaya/etcdv3_service_discovery.h"
#include "pitaya/etcdv3_service_discovery/config.h"

#include "mock_etcd_client.h"
#include <thread>

using namespace pitaya;
using namespace etcdv3_service_discovery;
using namespace ::testing;

class Etcdv3ServiceDiscoveryTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        _mockEtcdClient = new MockEtcdClient();

        _server = Server(Server::Kind::Backend, "my-server-id", "connector");

        _config = Config();
        _config.etcdPrefix = "pitaya/";
        _config.heartbeatTTLSec = std::chrono::seconds(5);
        _config.syncServersIntervalSec = std::chrono::seconds(4);

        //        spdlog::set_level(spdlog::level::debug);
        spdlog::set_level(spdlog::level::off);
    }

    void TearDown() override
    {
        _serviceDiscovery.reset();
        _mockEtcdClient = nullptr;
    }

protected:
    std::unique_ptr<etcdv3_service_discovery::Etcdv3ServiceDiscovery> _serviceDiscovery;
    Server _server;
    MockEtcdClient* _mockEtcdClient;
    Config _config;
};

static LeaseGrantResponse
NewSuccessfullLeaseGrantResponse(int64_t leaseId)
{
    LeaseGrantResponse leaseGrantRes;
    leaseGrantRes.ok = true;
    leaseGrantRes.leaseId = 1293102390123;
    return leaseGrantRes;
}

static SetResponse
NewSuccessfullSetResponse()
{
    SetResponse res;
    res.ok = true;
    return res;
}

TEST_F(Etcdv3ServiceDiscoveryTest, NoneIsReturnedWhenThereAreNoServers)
{
    auto leaseGrantRes = NewSuccessfullLeaseGrantResponse(12931092301293);
    auto setRes = NewSuccessfullSetResponse();

    EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix)));

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
            .WillOnce(Return(leaseGrantRes));
        EXPECT_CALL(*_mockEtcdClient, Set(_, _, Eq(leaseGrantRes.leaseId)))
            .WillOnce(Return(setRes));
        EXPECT_CALL(*_mockEtcdClient, LeaseRevoke(Eq(leaseGrantRes.leaseId)));
    }

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(leaseGrantRes.leaseId), _));
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
    }

    _serviceDiscovery = std::unique_ptr<Etcdv3ServiceDiscovery>(
        new Etcdv3ServiceDiscovery(_config, _server, std::unique_ptr<EtcdClient>(_mockEtcdClient)));

    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);

    auto server = _serviceDiscovery->GetServerById("random-id");
    ASSERT_EQ(server, boost::none);
}

TEST_F(Etcdv3ServiceDiscoveryTest, WatchesForKeysAddedAndRemoved)
{
    auto leaseGrantRes = NewSuccessfullLeaseGrantResponse(129310);
    auto setRes = NewSuccessfullSetResponse();

    ListResponse listRes;
    listRes.ok = true;

    LeaseRevokeResponse revokeRes;
    revokeRes.ok = true;

    EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix))).WillRepeatedly(Return(listRes));

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
            .WillOnce(Return(leaseGrantRes));
        EXPECT_CALL(*_mockEtcdClient, Set(_, _, Eq(leaseGrantRes.leaseId)))
            .WillOnce(Return(setRes));
        EXPECT_CALL(*_mockEtcdClient, LeaseRevoke(Eq(leaseGrantRes.leaseId)))
            .WillOnce(Return(revokeRes));
    }

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(leaseGrantRes.leaseId), _));
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
    }

    _serviceDiscovery = std::unique_ptr<Etcdv3ServiceDiscovery>(
        new Etcdv3ServiceDiscovery(_config, _server, std::unique_ptr<EtcdClient>(_mockEtcdClient)));

    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);

    pitaya::WatchResponse watchRes;
    watchRes.ok = true;

    {
        watchRes.action = "create";
        watchRes.key = "pitaya/servers/mytype/myid";
        watchRes.value = "{\"id\": \"myid\", \"type\": \"mytype\"}";
        _mockEtcdClient->onWatch(watchRes);

        auto server = _serviceDiscovery->GetServerById("myid");
        ASSERT_EQ(server, Server(Server::Kind::Backend, "myid", "mytype"));
    }

    for (int i = 0; i < 2; ++i) {
        watchRes.action = "delete";
        watchRes.key = "pitaya/servers/mytype/myid";
        watchRes.value = "{\"id\": \"myid\", \"type\": \"mytype\"}";
        _mockEtcdClient->onWatch(watchRes);

        auto server = _serviceDiscovery->GetServerById("myid");
        ASSERT_EQ(server, boost::none);
    }
}

TEST_F(Etcdv3ServiceDiscoveryTest, SynchronizesServersEveryInterval)
{
    // Will synchronize servers with etcd manually every 2 seconds
    _config.syncServersIntervalSec = std::chrono::seconds(1);

    auto leaseGrantRes = NewSuccessfullLeaseGrantResponse(129310);
    auto setRes = NewSuccessfullSetResponse();

    ListResponse firstListRes;
    firstListRes.ok = true;
    firstListRes.keys = {
        "other-prefix/servers/connector/super-id",
        "pitaya/servers/connector/myid",
        "pitaya/servers/room/awesome-id",
    };

    ListResponse secondListRes;
    secondListRes.ok = true;
    secondListRes.keys = {
        "other-prefix/servers/connector/super-id",
        "pitaya/servers/room/awesome-id",
    };

    LeaseRevokeResponse revokeRes;
    revokeRes.ok = true;

    EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix)))
        .Times(2)
        .WillOnce(Return(firstListRes))
        .WillOnce(Return(secondListRes));

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
            .WillOnce(Return(leaseGrantRes));
        EXPECT_CALL(*_mockEtcdClient, Set(_, _, Eq(leaseGrantRes.leaseId)))
            .WillOnce(Return(setRes));
        EXPECT_CALL(*_mockEtcdClient, LeaseRevoke(Eq(leaseGrantRes.leaseId)))
            .WillOnce(Return(revokeRes));
    }

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(leaseGrantRes.leaseId), _));
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
    }

    {
        GetResponse firstGetRes;
        firstGetRes.ok = true;
        firstGetRes.value = "{\"id\": \"myid\", \"type\": \"connector\"}";

        GetResponse secondGetRes;
        secondGetRes.ok = true;
        secondGetRes.value = "{\"id\": \"awesome-id\", \"type\": \"room\", \"frontend\": true}";

        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, Get("pitaya/servers/connector/myid"))
            .WillOnce(Return(firstGetRes));
        EXPECT_CALL(*_mockEtcdClient, Get("pitaya/servers/room/awesome-id"))
            .WillOnce(Return(secondGetRes));
    }

    _serviceDiscovery = std::unique_ptr<Etcdv3ServiceDiscovery>(
        new Etcdv3ServiceDiscovery(_config, _server, std::unique_ptr<EtcdClient>(_mockEtcdClient)));

    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);

    {
        auto server = _serviceDiscovery->GetServerById("myid");
        EXPECT_TRUE(server);
        EXPECT_EQ(server.value(), Server(Server::Kind::Backend, "myid", "connector"));
    }
    {
        auto server = _serviceDiscovery->GetServerById("awesome-id");
        EXPECT_TRUE(server);
        EXPECT_EQ(server.value(), Server(Server::Kind::Frontend, "awesome-id", "room"));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    {
        auto server = _serviceDiscovery->GetServerById("myid");
        EXPECT_EQ(server, boost::none);
    }
    {
        auto server = _serviceDiscovery->GetServerById("awesome-id");
        EXPECT_TRUE(server);
        EXPECT_EQ(server.value(), Server(Server::Kind::Frontend, "awesome-id", "room"));
    }
    {
        auto server = _serviceDiscovery->GetServerById("super-id");
        EXPECT_EQ(server, boost::none);
    }
}
