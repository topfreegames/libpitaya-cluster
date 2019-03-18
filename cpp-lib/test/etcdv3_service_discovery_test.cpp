#include "pitaya/etcdv3_service_discovery.h"
#include "pitaya/etcdv3_service_discovery/config.h"
#include "mock_etcd_client.h"

#include "pitaya.h"
#include "test_common.h"

using namespace pitaya;
using namespace etcdv3_service_discovery;
using namespace ::testing;

class Etcdv3ServiceDiscoveryTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        _mockEtcdClient = new MockEtcdClient();

        _server = Server();
        _server.frontend = false;
        _server.id = "my-server-id";
        _server.type = "connector";

        _config = Config();
        _config.etcdPrefix = "/pitaya";
        _config.heartbeatTTLSec = std::chrono::seconds(5);
        _config.syncServersIntervalSec = std::chrono::seconds(4);

        spdlog::set_level(spdlog::level::critical);
    }

    void TearDown() override { }

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

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseGrant(Eq(_config.heartbeatTTLSec)))
            .WillOnce(Return(leaseGrantRes));
        EXPECT_CALL(*_mockEtcdClient, Set(_, _, Eq(leaseGrantRes.leaseId)))
            .WillOnce(Return(setRes));
        EXPECT_CALL(*_mockEtcdClient, List(Eq(_config.etcdPrefix)));
        EXPECT_CALL(*_mockEtcdClient, LeaseRevoke(Eq(leaseGrantRes.leaseId)));
    }

    {
        InSequence seq;
        EXPECT_CALL(*_mockEtcdClient, LeaseKeepAlive(Eq(leaseGrantRes.leaseId), _));
        EXPECT_CALL(*_mockEtcdClient, StopLeaseKeepAlive());
    }

    _serviceDiscovery = std::unique_ptr<Etcdv3ServiceDiscovery>(new Etcdv3ServiceDiscovery(_config, _server, std::unique_ptr<EtcdClient>(_mockEtcdClient)));

    ASSERT_NE(_mockEtcdClient->onWatch, nullptr);

    auto server = _serviceDiscovery->GetServerById("random-id");
    ASSERT_EQ(server, boost::none);
}
