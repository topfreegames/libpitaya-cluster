#include "test_common.h"

#include "pitaya.h"
#include "pitaya/etcdv3_service_discovery.h"
#include "pitaya/etcdv3_service_discovery/config.h"

#include "mock_etcd_client.h"
#include "mock_service_discovery.h"
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

static ListResponse
NewListResponse(std::vector<std::string> keys = std::vector<std::string>())
{
    ListResponse res;
    if (keys.empty()) {
        res.ok = false;
    } else {
        res.ok = true;
        res.keys = std::move(keys);
    }
    return res;
}

static SetResponse
NewSuccessfullSetResponse()
{
    SetResponse res;
    res.ok = true;
    return res;
}

TEST_F(Etcdv3ServiceDiscoveryTest, ThrowsIfInvalidServerPassed)
{
    struct
    {
        std::string serverId;
        std::string serverType;
    } arr[] = {
        { "", "" },
        { "myid", "" },
        { "", "mytype" },
    };

    auto MakeServiceDiscovery =
        [this](const std::string& serverId,
               const std::string& serverType) -> std::unique_ptr<Etcdv3ServiceDiscovery> {
        return std::unique_ptr<Etcdv3ServiceDiscovery>(
            new Etcdv3ServiceDiscovery(_config,
                                       Server(Server::Kind::Frontend, serverId, serverType),
                                       std::unique_ptr<EtcdClient>(new MockEtcdClient())));
    };

    for (const auto& pair : arr) {
        EXPECT_THROW(MakeServiceDiscovery(pair.serverId, pair.serverType), PitayaException);
    }
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

    auto firstListRes = NewListResponse({
        "other-prefix/servers/connector/super-id",
        "pitaya/servers/connector/myid",
        "pitaya/servers/room/awesome-id",
    });

    auto secondListRes = NewListResponse({
        "other-prefix/servers/connector/super-id",
        "pitaya/servers/room/awesome-id",
    });

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

TEST_F(Etcdv3ServiceDiscoveryTest, ListenersCanBeAddedAndRemoved)
{
    // Will synchronize servers with etcd manually every 2 seconds
    _config.syncServersIntervalSec = std::chrono::seconds(1);

    auto firstListRes = NewListResponse({
        "other-prefix/servers/connector/super-id",
        "pitaya/servers/connector/myid",
        "pitaya/servers/room/awesome-id",
    });

    auto secondListRes = NewListResponse({
        "other-prefix/servers/connector/super-id",
        "pitaya/servers/room/awesome-id",
    });

    LeaseRevokeResponse revokeRes;
    revokeRes.ok = true;

    EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix)))
        .Times(2)
        .WillOnce(Return(firstListRes))
        .WillOnce(Return(secondListRes));

    auto leaseGrantRes = NewSuccessfullLeaseGrantResponse(129310);
    auto setRes = NewSuccessfullSetResponse();

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

    // As soon as we add the listener, it will be called with all of the current servers
    // on the service discovery. After it will be called whenever a server is removed or added.
    auto listener =
        std::unique_ptr<MockServiceDiscoveryListener>(new MockServiceDiscoveryListener());

    {
        InSequence seq;
        EXPECT_CALL(*listener,
                    ServerAdded(AnyOf(Server(Server::Kind::Frontend, "awesome-id", "room"),
                                      Server(Server::Kind::Backend, "myid", "connector"))))
            .Times(2);
        EXPECT_CALL(*listener, ServerRemoved(Server(Server::Kind::Backend, "myid", "connector")));
    }

    _serviceDiscovery = std::unique_ptr<Etcdv3ServiceDiscovery>(
        new Etcdv3ServiceDiscovery(_config, _server, std::unique_ptr<EtcdClient>(_mockEtcdClient)));

    _serviceDiscovery->AddListener(listener.get());

    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // Simulate a new server removed with on watch
    pitaya::WatchResponse watchRes;
    watchRes.ok = true;
    watchRes.action = "delete";
    watchRes.key = "pitaya/servers/room/awesome-id";

    _serviceDiscovery->RemoveListener(listener.get());
    _mockEtcdClient->onWatch(watchRes);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // We removed the listener, therefore we do not receive a notification that the
    // server was removed, however we can check that by trying the get the server by id.
    auto server = _serviceDiscovery->GetServerById("awesome-id");
    EXPECT_EQ(server, boost::none);
}

TEST_F(Etcdv3ServiceDiscoveryTest, ServerIsIgnoredInSyncServersIfGetFromEtcdFails)
{
    // Will synchronize servers with etcd manually every 2 seconds
    _config.syncServersIntervalSec = std::chrono::seconds(1);

    auto firstListRes = NewListResponse({
        "other-prefix/servers/connector/super-id",
        "pitaya/servers/connector/myid",
        "pitaya/servers/room/awesome-id",
    });

    LeaseRevokeResponse revokeRes;
    revokeRes.ok = true;

    EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix))).WillOnce(Return(firstListRes));

    auto leaseGrantRes = NewSuccessfullLeaseGrantResponse(129310);
    auto setRes = NewSuccessfullSetResponse();

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
        firstGetRes.ok = false;

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
        EXPECT_FALSE(server);
    }
    {
        auto server = _serviceDiscovery->GetServerById("awesome-id");
        EXPECT_TRUE(server);
        EXPECT_EQ(server.value(), Server(Server::Kind::Frontend, "awesome-id", "room"));
    }
}
