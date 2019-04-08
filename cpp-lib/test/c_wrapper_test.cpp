#include "test_common.h"

#include "pitaya.h"
#include "pitaya/c_wrapper.h"
#include "pitaya/constants.h"
#include "pitaya/grpc/rpc_client.h"
#include "pitaya/protos/pitaya_mock.grpc.pb.h"

#include "mock_binding_storage.h"
#include "mock_etcd_client.h"
#include "mock_service_discovery.h"
#include <cpprest/json.h>
#include <regex>

using namespace testing;
namespace constants = pitaya::constants;

class CWrapperTest : public testing::Test
{
public:
    void SetUp() override {}

    void TearDown() override {}

protected:
};

TEST_F(CWrapperTest, CanInitializeAndTerminate)
{
    CGrpcConfig grpcConfig = {};
    grpcConfig.connectionTimeoutSec = 4;
    grpcConfig.host = "127.0.0.1";
    grpcConfig.port = 40000;
    grpcConfig.serverMaxNumberOfRpcs = 100;
    grpcConfig.serverShutdownDeadlineMs = 3000;

    CSDConfig sdConfig = {};
    sdConfig.endpoints = "http://127.0.0.1:2379";
    sdConfig.etcdPrefix = "pitaya/";
    sdConfig.heartbeatTTLSec = 5;
    sdConfig.logHeartbeat = false;
    sdConfig.logServerSync = false;
    sdConfig.logServerDetails = false;
    sdConfig.syncServersIntervalSec = 20;

    CServer server = {};
    server.frontend = true;
    server.id = (char*)"id";
    server.type = (char*)"type";

    for (size_t i = 0; i < 20; ++i) {
        bool ok = tfg_pitc_InitializeWithGrpc(
            &grpcConfig, &sdConfig, &server, LogLevel_Critical, nullptr);

        tfg_pitc_Terminate();
    }
}
