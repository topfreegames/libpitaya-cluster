using System;
using System.Collections.Generic;
using System.Threading;
using NPitaya;
using NPitaya.Metrics;
using Xunit;

namespace NPitayaTest.Tests.Integration
{
    public class NatsCluster
    {
        [Fact]
        public void Can_Fail_Initialization_Multiple_Times()
        {
            var natsConfig = new NatsConfig(
                endpoint: "http://127.0.0.1:4222",
                connectionTimeoutMs: 1000,
                requestTimeoutMs: 5000,
                serverShutdownDeadlineMs: 10 * 1000,
                serverMaxNumberOfRpcs: int.MaxValue,
                maxConnectionRetries: 3,
                maxPendingMessages: 100);

            var sdCfg = new SDConfig(
                endpoints: "127.0.0.1:123123123",
                etcdPrefix: "pitaya/",
                heartbeatTTLSec: 10,
                logHeartbeat: false,
                logServerSync: false,
                logServerDetails: false,
                syncServersIntervalSec: 10,
                maxNumberOfRetries: 0
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
                    PitayaCluster.Initialize(natsConfig, sdCfg, server, NativeLogLevel.Debug);
                });
            }

            PitayaCluster.Terminate();
        }

        [Fact]
        public void Can_Be_Initialized_And_Terminated_Multiple_Times()
        {
            var natsConfig = new NatsConfig(
                endpoint: "http://127.0.0.1:4222",
                connectionTimeoutMs: 1000,
                requestTimeoutMs: 5000,
                serverShutdownDeadlineMs: 10 * 1000,
                serverMaxNumberOfRpcs: int.MaxValue,
                maxConnectionRetries: 3,
                maxPendingMessages: 100);

            var sdCfg = new SDConfig(
                endpoints: "http://127.0.0.1:2379",
                etcdPrefix: "pitaya/",
                heartbeatTTLSec: 10,
                logHeartbeat: false,
                logServerSync: false,
                logServerDetails: false,
                syncServersIntervalSec: 10,
                maxNumberOfRetries: 0
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
                PitayaCluster.Initialize(natsConfig, sdCfg, server, NativeLogLevel.Debug);
                PitayaCluster.Terminate();
            }
        }

        [Fact]
        public void Can_Be_Initialized_And_Report_Metrics_In_Parallel()
        {
            var natsConfig = new NatsConfig(
                endpoint: "http://127.0.0.1:4222",
                connectionTimeoutMs: 1000,
                requestTimeoutMs: 5000,
                serverShutdownDeadlineMs: 10 * 1000,
                serverMaxNumberOfRpcs: int.MaxValue,
                maxConnectionRetries: 3,
                maxPendingMessages: 100);

            var sdCfg = new SDConfig(
                endpoints: "http://127.0.0.1:2379",
                etcdPrefix: "pitaya/",
                heartbeatTTLSec: 10,
                logHeartbeat: false,
                logServerSync: false,
                logServerDetails: false,
                syncServersIntervalSec: 10,
                maxNumberOfRetries: 0
            );

            var server = new Server(
                id: "myserverid",
                type: "myservertype",
                metadata: "",
                hostname: "",
                frontend: false
            );

            PitayaCluster.Initialize(natsConfig, sdCfg, server, NativeLogLevel.Debug);
            var statsdMR = new StatsdMetricsReporter("localhost", 5000, "game");
            MetricsReporters.AddMetricReporter(statsdMR);
            var prometheusMR = new PrometheusMetricsReporter("default", "game", 9090);
            MetricsReporters.AddMetricReporter(prometheusMR);

            for (var i = 0; i < 5; ++i)
            {
                new Thread(() =>
                {
                    Thread.CurrentThread.IsBackground = true;
                    /* run your code here */
                    MetricsReporters.ReportTimer(new Dictionary<string, string>(), 0);
                }).Start();
            }
        }
    }
}