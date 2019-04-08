using NPitaya;
using Xunit;

namespace NPitayaTest.Tests.Integration
{
    public class Cluster
    {
        [Fact]
        public void Can_Fail_Initialization_Multiple_Times()
        {
            var grpcConfig = new GrpcConfig(
                host: "127.0.0.1",
                port: 40405,
                connectionTimeoutSec: 2,
                serverShutdownDeadlineMs: 2000,
                serverMaxNumberOfRpcs: 100
            );

            var sdCfg = new SDConfig(
                endpoints: "127.0.0.1:123123123",
                etcdPrefix: "pitaya/",
                heartbeatTTLSec: 10,
                logHeartbeat: false,
                logServerSync: false,
                logServerDetails: false,
                syncServersIntervalSec: 10
            );

            var server = new Server(
                id: "id",
                type: "type",
                metadata: "",
                hostname: "",
                frontend: false
            );

            for (var i = 0; i < 10; ++i)
            {
                Assert.Throws<PitayaException>(() =>
                {
                    PitayaCluster.Initialize(grpcConfig, sdCfg, server, NativeLogLevel.Debug);
                });
            }

            PitayaCluster.Terminate();
        }

        [Fact]
        public void Can_Be_Initialized_And_Terminated_Multiple_Times()
        {
            var grpcConfig = new GrpcConfig(
                host: "127.0.0.1",
                port: 40405,
                connectionTimeoutSec: 2,
                serverShutdownDeadlineMs: 2000,
                serverMaxNumberOfRpcs: 100
            );

            var sdCfg = new SDConfig(
                endpoints: "http://127.0.0.1:2379",
                etcdPrefix: "pitaya/",
                heartbeatTTLSec: 10,
                logHeartbeat: false,
                logServerSync: false,
                logServerDetails: false,
                syncServersIntervalSec: 10
            );

            var server = new Server(
                id: "myserverid",
                type: "myservertype",
                metadata: "",
                hostname: "",
                frontend: false
            );

            for (var i = 0; i < 2; ++i)
            {
                PitayaCluster.Initialize(grpcConfig, sdCfg, server, NativeLogLevel.Debug);
                PitayaCluster.Terminate();
            }
        }
    }
}