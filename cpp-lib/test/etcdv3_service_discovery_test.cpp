#include "test_common.h"

#include "pitaya.h"
#include "pitaya/etcd_config.h"
#include "pitaya/etcdv3_service_discovery.h"

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

        _config = EtcdServiceDiscoveryConfig();
        _config.etcdPrefix = "pitaya/";
        _config.heartbeatTTLSec = std::chrono::seconds(5);
        _config.syncServersIntervalSec = std::chrono::seconds(4);
    }
    
    std::unique_ptr<etcdv3_service_discovery::Etcdv3ServiceDiscovery> CreateServiceDiscovery()
    {
        return std::unique_ptr<Etcdv3ServiceDiscovery>(
            new Etcdv3ServiceDiscovery(_config, _server, std::unique_ptr<EtcdClient>(_mockEtcdClient)));
    }

    void TearDown() override
    {
        _mockEtcdClient = nullptr;
    }

protected:
    Server _server;
    MockEtcdClient* _mockEtcdClient;
    EtcdServiceDiscoveryConfig _config;
};

static LeaseGrantResponse
NewSuccessfullLeaseGrantResponse(int64_t leaseId)
{
    LeaseGrantResponse leaseGrantRes;
    leaseGrantRes.ok = true;
    leaseGrantRes.leaseId = leaseId;
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

    EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix + "servers/metagame/")));

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
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), _, _));
        EXPECT_CALL(*_mockEtcdClient, CancelWatch());
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
    }

    auto serviceDiscovery = CreateServiceDiscovery();

    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);

    auto server = serviceDiscovery->GetServerById("random-id");
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

    EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix + "servers/metagame/"))).WillRepeatedly(Return(listRes));

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
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), _, _));
        EXPECT_CALL(*_mockEtcdClient, CancelWatch());
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
    }

    auto serviceDiscovery = CreateServiceDiscovery();

    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);

    pitaya::WatchResponse watchRes;
    watchRes.ok = true;

    {
        watchRes.action = "create";
        watchRes.key = "pitaya/servers/mytype/myid";
        watchRes.value = "{\"id\": \"myid\", \"type\": \"mytype\"}";
        _mockEtcdClient->onWatch(watchRes);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto server = serviceDiscovery->GetServerById("myid");
        ASSERT_EQ(server, Server(Server::Kind::Backend, "myid", "mytype"));
    }

    for (int i = 0; i < 2; ++i) {
        watchRes.action = "delete";
        watchRes.key = "pitaya/servers/mytype/myid";
        watchRes.value = "{\"id\": \"myid\", \"type\": \"mytype\"}";
        _mockEtcdClient->onWatch(watchRes);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto server = serviceDiscovery->GetServerById("myid");
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

    EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix + "servers/metagame/")))
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
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), _, _));
        EXPECT_CALL(*_mockEtcdClient, CancelWatch());
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

    auto serviceDiscovery = CreateServiceDiscovery();

    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    {
        auto server = serviceDiscovery->GetServerById("myid");
        EXPECT_TRUE(server);
        if (server) {
            EXPECT_EQ(server.value(), Server(Server::Kind::Backend, "myid", "connector"));
        }
    }
    {
        auto server = serviceDiscovery->GetServerById("awesome-id");
        EXPECT_TRUE(server);
        if (server) {
            EXPECT_EQ(server.value(), Server(Server::Kind::Frontend, "awesome-id", "room"));
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    {
        auto server = serviceDiscovery->GetServerById("myid");
        EXPECT_EQ(server, boost::none);
    }
    {
        auto server = serviceDiscovery->GetServerById("awesome-id");
        EXPECT_TRUE(server);
        if (server) {
            EXPECT_EQ(server.value(), Server(Server::Kind::Frontend, "awesome-id", "room"));
        }
    }
    {
        auto server = serviceDiscovery->GetServerById("super-id");
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

    EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix + "servers/metagame/")))
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
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), _, _));
        EXPECT_CALL(*_mockEtcdClient, CancelWatch());
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

    auto serviceDiscovery = CreateServiceDiscovery();
    serviceDiscovery->AddListener(listener.get());

    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // Simulate a new server removed with on watch
    pitaya::WatchResponse watchRes;
    watchRes.ok = true;
    watchRes.action = "delete";
    watchRes.key = "pitaya/servers/room/awesome-id";

    serviceDiscovery->RemoveListener(listener.get());
    _mockEtcdClient->onWatch(watchRes);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // We removed the listener, therefore we do not receive a notification that the
    // server was removed, however we can check that by trying the get the server by id.
    auto server = serviceDiscovery->GetServerById("awesome-id");
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

    EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix + "servers/metagame/"))).WillOnce(Return(firstListRes));

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
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), _, _));
        EXPECT_CALL(*_mockEtcdClient, CancelWatch());
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

    auto serviceDiscovery = CreateServiceDiscovery();

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);

    {
        auto server = serviceDiscovery->GetServerById("myid");
        EXPECT_FALSE(server);
    }
    {
        auto server = serviceDiscovery->GetServerById("awesome-id");
        EXPECT_TRUE(server);
        if (server) {
            EXPECT_EQ(server.value(), Server(Server::Kind::Frontend, "awesome-id", "room"));
        }
    }
}

TEST_F(Etcdv3ServiceDiscoveryTest, ServerIsIgnoredInSyncServersIfTheServerTypeIsFilteredOut)
{
    // Will synchronize servers with etcd manually every 2 seconds
    _config.syncServersIntervalSec = std::chrono::seconds(1);
    // We only want to keep servers that are of the following types
    _config.serverTypeFilters = {
        "server-type1", "server-type2", "server-type3",
    };
    
    auto listRes = NewListResponse({
        "pitaya/servers/room/awesome-id1",
        "pitaya/servers/server-type1/awesome-id2",
        "pitaya/servers/connector/myid",
        "pitaya/servers/server-type3/awesome-id3",
        "other-prefix/servers/connector/super-id4",
        "pitaya/servers/server-type2/awesome-id5",
    });
    
    LeaseRevokeResponse revokeRes;
    revokeRes.ok = true;
    
    EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix + "servers/metagame/"))).WillOnce(Return(listRes));
    
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
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), _, _));
        EXPECT_CALL(*_mockEtcdClient, CancelWatch());
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
    }

    {
        GetResponse firstGetRes;
        firstGetRes.ok = true;
        firstGetRes.value = "{\"id\": \"awesome-id2\", \"type\": \"server-type1\", \"frontend\": true}";

        GetResponse secondGetRes;
        secondGetRes.ok = true;
        secondGetRes.value = "{\"id\": \"awesome-id3\", \"type\": \"server-type3\", \"frontend\": true}";

        GetResponse thirdGetRes;
        thirdGetRes.ok = true;
        thirdGetRes.value = "{\"id\": \"awesome-id5\", \"type\": \"server-type2\", \"frontend\": true}";

        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, Get("pitaya/servers/server-type1/awesome-id2"))
        .WillOnce(Return(firstGetRes));
        EXPECT_CALL(*_mockEtcdClient, Get("pitaya/servers/server-type3/awesome-id3"))
        .WillOnce(Return(secondGetRes));
        EXPECT_CALL(*_mockEtcdClient, Get("pitaya/servers/server-type2/awesome-id5"))
        .WillOnce(Return(thirdGetRes));
    }

    auto serviceDiscovery = CreateServiceDiscovery();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);
    
    // Check that the necessary servers are there
    for (const auto& key : { "awesome-id2", "awesome-id3", "awesome-id5" }) {
        auto server = serviceDiscovery->GetServerById(key);
        EXPECT_TRUE(server) << "Key " << key << " should be present";
    }
    
    // Check that the filtered servers are not there
    for (const auto& key : { "awesome-id1", "myid", "super-id4" }) {
        auto server = serviceDiscovery->GetServerById(key);
        EXPECT_EQ(server, boost::none) << "Key " << key << " should NOT be present";
    }
}

ACTION_TEMPLATE(SaveFunction,
                HAS_2_TEMPLATE_PARAMS(int, k, int, kk),
                AND_1_VALUE_PARAMS(pointer))
{
    *pointer = std::get<kk>(args);
}


TEST_F(Etcdv3ServiceDiscoveryTest, ReconnectsIfLosesConnectionToEtcd)
{
    // First lease grant will succeed but the second will fail.
    auto firstLeaseGrantRes = NewSuccessfullLeaseGrantResponse(712381283);
    auto setRes = NewSuccessfullSetResponse();

    ListResponse listRes;
    listRes.ok = true;
    
    LeaseRevokeResponse revokeRes;
    revokeRes.ok = true;
    
    EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix + "servers/metagame/"))).WillRepeatedly(Return(listRes));
    
    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
        .WillOnce(Return(firstLeaseGrantRes));
        EXPECT_CALL(*_mockEtcdClient, Set(_, _, Eq(firstLeaseGrantRes.leaseId)))
        .WillOnce(Return(setRes));
    }
    
    std::function<void(std::exception_ptr)> onLeaseKeepAliveExit;
    
    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), firstLeaseGrantRes.leaseId, _))
        .WillOnce(SaveFunction<1,2>(&onLeaseKeepAliveExit));
        EXPECT_CALL(*_mockEtcdClient, CancelWatch());
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
    }
    
    auto secondLeaseGrantRes = NewSuccessfullLeaseGrantResponse(10101010);
    {
        // These are called after the disconnection
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
        .WillOnce(Return(secondLeaseGrantRes));
        EXPECT_CALL(*_mockEtcdClient, Set(_, _, Eq(secondLeaseGrantRes.leaseId)))
        .WillOnce(Return(setRes));
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), secondLeaseGrantRes.leaseId, _))
        .WillOnce(SaveFunction<1,2>(&onLeaseKeepAliveExit));
        EXPECT_CALL(*_mockEtcdClient, LeaseRevoke(Eq(secondLeaseGrantRes.leaseId)))
        .WillOnce(Return(revokeRes));
    }

    _config.syncServersIntervalSec = std::chrono::seconds(1);
    auto serviceDiscovery = CreateServiceDiscovery();
    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);
    
    pitaya::WatchResponse watchRes;
    watchRes.ok = true;

    {
        watchRes.action = "create";
        watchRes.key = "pitaya/servers/mytype/myid";
        watchRes.value = "{\"id\": \"myid\", \"type\": \"mytype\"}";
        _mockEtcdClient->onWatch(watchRes);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto server = serviceDiscovery->GetServerById("myid");
        ASSERT_EQ(server, Server(Server::Kind::Backend, "myid", "mytype"));
    }
    
    onLeaseKeepAliveExit(std::make_exception_ptr(std::runtime_error("some_error")));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

TEST_F(Etcdv3ServiceDiscoveryTest, SyncIsStillCalledAfterReconnection)
{
    // First lease grant will succeed but the second will fail.
    auto firstLeaseGrantRes = NewSuccessfullLeaseGrantResponse(712381283);
    auto setRes = NewSuccessfullSetResponse();
    
    ListResponse listRes;
    listRes.ok = true;
    
    LeaseRevokeResponse revokeRes;
    revokeRes.ok = true;
    
    _config.syncServersIntervalSec = std::chrono::seconds(1);
    
    {
        // Synchronization will be called by the worker even after reconnection
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix + "servers/metagame/")))
        .WillOnce(Return(listRes)) // First time is called in the initial connection
        .WillOnce(Return(listRes)) // Second time is called after 1 second
        .WillOnce(Return(listRes)) // Third time is called right after reconnection
        .WillOnce(Return(listRes)) // Fourth time is called 2 second after reconnection
        .WillOnce(Return(listRes)) // Fifth time is called 3 seconds after reconnection
        .WillOnce(Return(listRes)); // Fifth time is called 4 seconds after reconnection
    }

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
        .WillOnce(Return(firstLeaseGrantRes));
        EXPECT_CALL(*_mockEtcdClient, Set(_, _, Eq(firstLeaseGrantRes.leaseId)))
        .WillOnce(Return(setRes));
    }
    
    std::function<void(std::exception_ptr)> onLeaseKeepAliveExit;
    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), firstLeaseGrantRes.leaseId, _))
        .WillOnce(SaveFunction<1,2>(&onLeaseKeepAliveExit));
        EXPECT_CALL(*_mockEtcdClient, CancelWatch());
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
    }
    
    auto secondLeaseGrantRes = NewSuccessfullLeaseGrantResponse(10101010);
    {
        // These are called after the disconnection
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
        .WillOnce(Return(secondLeaseGrantRes));
        EXPECT_CALL(*_mockEtcdClient, Set(_, _, Eq(secondLeaseGrantRes.leaseId)))
        .WillOnce(Return(setRes));
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), secondLeaseGrantRes.leaseId, _))
        .WillOnce(SaveFunction<1,2>(&onLeaseKeepAliveExit));
        EXPECT_CALL(*_mockEtcdClient, LeaseRevoke(Eq(secondLeaseGrantRes.leaseId)))
        .WillOnce(Return(revokeRes));
    }
    
    _config.syncServersIntervalSec = std::chrono::seconds(1);
    auto serviceDiscovery = CreateServiceDiscovery();
    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);
    
    pitaya::WatchResponse watchRes;
    watchRes.ok = true;
    
    {
        watchRes.action = "create";
        watchRes.key = "pitaya/servers/mytype/myid";
        watchRes.value = "{\"id\": \"myid\", \"type\": \"mytype\"}";
        _mockEtcdClient->onWatch(watchRes);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        auto server = serviceDiscovery->GetServerById("myid");
        ASSERT_EQ(server, Server(Server::Kind::Backend, "myid", "mytype"));
    }
    
    onLeaseKeepAliveExit(std::make_exception_ptr(std::runtime_error("some_error")));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(4500)); // Sleeping a little more than 4 seconds should call List five times
}

TEST_F(Etcdv3ServiceDiscoveryTest, FailsAfterMaxRetrieUsingExponentialBackoff)
{   
    std::function<void(std::exception_ptr)> onLeaseKeepAliveExit;
    ASSERT_EXIT({
    // lease response with failure
    LeaseGrantResponse failedLeaseGrantRes;
    failedLeaseGrantRes.ok = false;

    // First lease grant will succeed but the second will fail.
    auto firstLeaseGrantRes = NewSuccessfullLeaseGrantResponse(712381283);
    auto setRes = NewSuccessfullSetResponse();

    ListResponse listRes;
    listRes.ok = true;

    

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix + "servers/metagame/")))
        .WillOnce(Return(listRes)); // First time is called in the initial connection
    }

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
        .WillOnce(Return(firstLeaseGrantRes));
        EXPECT_CALL(*_mockEtcdClient, Set(_, _, Eq(firstLeaseGrantRes.leaseId)))
        .WillOnce(Return(setRes));
    }

    

    {
        // These are called after the disconnection
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
        // fails two times to grant lease
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
        .WillOnce(Return(failedLeaseGrantRes))
        .WillOnce(Return(failedLeaseGrantRes));
    }

    {
        // These are called after the disconnection
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
        // fails two times to grant lease
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
        .WillOnce(Return(failedLeaseGrantRes))
        .WillOnce(Return(failedLeaseGrantRes));
    }
    

    {
        InSequence seq;
        
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), 712381283, _))
        .WillOnce(SaveFunction<1,2>(&onLeaseKeepAliveExit));
        EXPECT_CALL(*_mockEtcdClient, LeaseRevoke(712381283));
        EXPECT_CALL(*_mockEtcdClient, CancelWatch());
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
    }

    _config.retryDelayMilliseconds = 100; // set retry delay (in milliseconds)
    _config.maxNumberOfRetries = 1;
    auto serviceDiscovery = CreateServiceDiscovery();
    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);

   

    onLeaseKeepAliveExit(std::make_exception_ptr(std::runtime_error("some_error")));
    std::this_thread::sleep_for(std::chrono::milliseconds(15100)); // Sleeping 1 second should retry 2 times.
    }, testing::KilledBySignal(SIGTERM), "");
}

TEST_F(Etcdv3ServiceDiscoveryTest, ReconnectsAfterSomeRetrieUsingExponentialBackoff)
{
    // lease response with failure
    LeaseGrantResponse failedLeaseGrantRes;
    failedLeaseGrantRes.ok = false;

    // First lease grant will succeed but the second will fail.
    auto firstLeaseGrantRes = NewSuccessfullLeaseGrantResponse(712381283);
    auto setRes = NewSuccessfullSetResponse();

    ListResponse listRes;
    listRes.ok = true;

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix + "servers/metagame/")))
        .WillOnce(Return(listRes)); // First time is called in the initial connection
    }

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
        .WillOnce(Return(firstLeaseGrantRes));
        EXPECT_CALL(*_mockEtcdClient, Set(_, _, Eq(firstLeaseGrantRes.leaseId)))
        .WillOnce(Return(setRes));
    }

    std::function<void(std::exception_ptr)> onLeaseKeepAliveExit;
    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), firstLeaseGrantRes.leaseId, _))
        .WillOnce(SaveFunction<1,2>(&onLeaseKeepAliveExit));
        EXPECT_CALL(*_mockEtcdClient, CancelWatch());
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
    }

    {
        // These are called after the disconnection
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
        // fails two times to grant lease
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
        .WillOnce(Return(failedLeaseGrantRes))
        .WillOnce(Return(failedLeaseGrantRes));
    }

    _config.retryDelayMilliseconds = 300; // set retry delay (in milliseconds)
    _config.maxNumberOfRetries = 5;
    auto serviceDiscovery = CreateServiceDiscovery();
    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);

    onLeaseKeepAliveExit(std::make_exception_ptr(std::runtime_error("some_error")));
    std::this_thread::sleep_for(std::chrono::milliseconds(1100)); // Sleeping 1 second should retry 2 times.

    // after two retries, make it work.
    auto secondLeaseGrantRes = NewSuccessfullLeaseGrantResponse(10101010);
    {
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
        .WillOnce(Return(secondLeaseGrantRes));
        EXPECT_CALL(*_mockEtcdClient, Set(_, _, Eq(secondLeaseGrantRes.leaseId)))
        .WillOnce(Return(setRes));
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(_config.heartbeatTTLSec.count()), secondLeaseGrantRes.leaseId, _))
        .WillOnce(SaveFunction<1,2>(&onLeaseKeepAliveExit));
        EXPECT_CALL(*_mockEtcdClient, LeaseRevoke(10101010));
    }
}
