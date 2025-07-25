using System;
using System.Collections.Generic;
using System.Diagnostics;
using NPitaya;
using Xunit;

namespace NPitayaTest.Tests.Integration
{
    public class NatsBackwardCompatibility
    {
        [Fact]
        public void Can_Use_Old_Constructor_With_MaxConnectionRetries()
        {
            var natsConfig = new NatsConfig(
                endpoint: "nats://localhost:4222",
                connectionTimeoutMs: 2000,
                requestTimeoutMs: 2000,
                serverShutdownDeadlineMs: 2000,
                serverMaxNumberOfRpcs: 500,
                maxConnectionRetries: 30, // Deprecated parameter
                maxPendingMessages: 100,
                reconnectBufSize: 8 * 1024 * 1024,
                maxReconnectionAttempts: 30,
                reconnectWaitInMs: 100,
                reconnectJitterInMs: 50,
                pingIntervalInMs: 1000,
                maxPingsOut: 2
            );

            // Verify the config was created correctly
            Assert.Equal("nats://localhost:4222", natsConfig.endpoint);
            Assert.Equal(2000, natsConfig.connectionTimeoutMs);
            Assert.Equal(30, natsConfig.maxConnectionRetries); // Should still be set
            Assert.Equal(30, natsConfig.maxReconnectionAttempts);
        }

        [Fact]
        public void Can_Use_New_Constructor_Without_MaxConnectionRetries()
        {
            var natsConfig = new NatsConfig(
                endpoint: "nats://localhost:4222",
                connectionTimeoutMs: 2000,
                requestTimeoutMs: 2000,
                serverShutdownDeadlineMs: 2000,
                serverMaxNumberOfRpcs: 500,
                maxPendingMessages: 100,
                reconnectBufSize: 8 * 1024 * 1024,
                maxReconnectionAttempts: 30,
                reconnectWaitInMs: 100,
                reconnectJitterInMs: 50,
                pingIntervalInMs: 1000,
                maxPingsOut: 2
            );

            Assert.Equal("nats://localhost:4222", natsConfig.endpoint);
            Assert.Equal(2000, natsConfig.connectionTimeoutMs);
            Assert.Equal(0, natsConfig.maxConnectionRetries); // Should be 0 for deprecated parameter
            Assert.Equal(30, natsConfig.maxReconnectionAttempts);
        }

        [Fact]
        public void Can_Use_Simple_Constructor_With_Defaults()
        {
            var natsConfig = new NatsConfig("nats://localhost:4222");

            Assert.Equal("nats://localhost:4222", natsConfig.endpoint);
            Assert.Equal(2000, natsConfig.connectionTimeoutMs);
            Assert.Equal(2000, natsConfig.requestTimeoutMs);
            Assert.Equal(2000, natsConfig.serverShutdownDeadlineMs);
            Assert.Equal(500, natsConfig.serverMaxNumberOfRpcs);
            Assert.Equal(0, natsConfig.maxConnectionRetries); // Should be 0 for deprecated parameter
            Assert.Equal(100, natsConfig.maxPendingMessages);
            Assert.Equal(8 * 1024 * 1024, natsConfig.reconnectBufSize);
            Assert.Equal(30, natsConfig.maxReconnectionAttempts);
            Assert.Equal(100, natsConfig.reconnectWaitInMs);
            Assert.Equal(50, natsConfig.reconnectJitterInMs);
            Assert.Equal(1000, natsConfig.pingIntervalInMs);
            Assert.Equal(2, natsConfig.maxPingsOut);
        }

        [Fact]
        public void Can_Use_Factory_Method_With_Defaults()
        {
            var natsConfig = NatsConfig.CreateWithDefaults("nats://localhost:4222");

            Assert.Equal("nats://localhost:4222", natsConfig.endpoint);
            Assert.Equal(2000, natsConfig.connectionTimeoutMs);
            Assert.Equal(2000, natsConfig.requestTimeoutMs);
            Assert.Equal(2000, natsConfig.serverShutdownDeadlineMs);
            Assert.Equal(500, natsConfig.serverMaxNumberOfRpcs);
            Assert.Equal(0, natsConfig.maxConnectionRetries); // Should be 0 for deprecated parameter
            Assert.Equal(100, natsConfig.maxPendingMessages);
            Assert.Equal(8 * 1024 * 1024, natsConfig.reconnectBufSize);
            Assert.Equal(30, natsConfig.maxReconnectionAttempts);
            Assert.Equal(100, natsConfig.reconnectWaitInMs);
            Assert.Equal(50, natsConfig.reconnectJitterInMs);
            Assert.Equal(1000, natsConfig.pingIntervalInMs);
            Assert.Equal(2, natsConfig.maxPingsOut);
        }

        [Fact]
        public void Can_Use_Constructor_With_Named_Parameters()
        {
            var natsConfig = new NatsConfig(
                endpoint: "nats://localhost:4222",
                connectionTimeoutMs: 5000,
                requestTimeoutMs: 3000,
                maxReconnectionAttempts: 50
            );

            Assert.Equal("nats://localhost:4222", natsConfig.endpoint);
            Assert.Equal(5000, natsConfig.connectionTimeoutMs);
            Assert.Equal(3000, natsConfig.requestTimeoutMs);
            Assert.Equal(2000, natsConfig.serverShutdownDeadlineMs); // Default
            Assert.Equal(500, natsConfig.serverMaxNumberOfRpcs); // Default
            Assert.Equal(0, natsConfig.maxConnectionRetries); // Should be 0 for deprecated parameter
            Assert.Equal(100, natsConfig.maxPendingMessages); // Default
            Assert.Equal(8 * 1024 * 1024, natsConfig.reconnectBufSize); // Default
            Assert.Equal(50, natsConfig.maxReconnectionAttempts); // Specified
            Assert.Equal(100, natsConfig.reconnectWaitInMs); // Default
            Assert.Equal(50, natsConfig.reconnectJitterInMs); // Default
            Assert.Equal(1000, natsConfig.pingIntervalInMs); // Default
            Assert.Equal(2, natsConfig.maxPingsOut); // Default
        }

        [Fact]
        public void Deprecated_Parameter_Is_Ignored_In_New_Constructors()
        {
            var natsConfig1 = new NatsConfig(
                endpoint: "nats://localhost:4222",
                connectionTimeoutMs: 2000,
                requestTimeoutMs: 2000,
                serverShutdownDeadlineMs: 2000,
                serverMaxNumberOfRpcs: 500,
                maxConnectionRetries: 10, // This should be ignored
                maxPendingMessages: 100,
                reconnectBufSize: 8 * 1024 * 1024,
                maxReconnectionAttempts: 30, // This should be used
                reconnectWaitInMs: 100,
                reconnectJitterInMs: 50,
                pingIntervalInMs: 1000,
                maxPingsOut: 2
            );

            var natsConfig2 = new NatsConfig(
                endpoint: "nats://localhost:4222",
                connectionTimeoutMs: 2000,
                requestTimeoutMs: 2000,
                serverShutdownDeadlineMs: 2000,
                serverMaxNumberOfRpcs: 500,
                maxPendingMessages: 100,
                reconnectBufSize: 8 * 1024 * 1024,
                maxReconnectionAttempts: 30, // This should be used
                reconnectWaitInMs: 100,
                reconnectJitterInMs: 50,
                pingIntervalInMs: 1000,
                maxPingsOut: 2
            );

            // Both configs should have the same maxReconnectionAttempts value
            Assert.Equal(natsConfig1.maxReconnectionAttempts, natsConfig2.maxReconnectionAttempts);
            Assert.Equal(30, natsConfig1.maxReconnectionAttempts);
            Assert.Equal(30, natsConfig2.maxReconnectionAttempts);
        }

        [Fact]
        public void All_Constructors_Produce_Valid_Configs()
        {
            var configs = new[]
            {
                // Old constructor with maxConnectionRetries
                new NatsConfig(
                    endpoint: "nats://localhost:4222",
                    connectionTimeoutMs: 2000,
                    requestTimeoutMs: 2000,
                    serverShutdownDeadlineMs: 2000,
                    serverMaxNumberOfRpcs: 500,
                    maxConnectionRetries: 30,
                    maxPendingMessages: 100,
                    reconnectBufSize: 8 * 1024 * 1024,
                    maxReconnectionAttempts: 30,
                    reconnectWaitInMs: 100,
                    reconnectJitterInMs: 50,
                    pingIntervalInMs: 1000,
                    maxPingsOut: 2
                ),

                // New constructor without maxConnectionRetries
                new NatsConfig(
                    endpoint: "nats://localhost:4222",
                    connectionTimeoutMs: 2000,
                    requestTimeoutMs: 2000,
                    serverShutdownDeadlineMs: 2000,
                    serverMaxNumberOfRpcs: 500,
                    maxPendingMessages: 100,
                    reconnectBufSize: 8 * 1024 * 1024,
                    maxReconnectionAttempts: 30,
                    reconnectWaitInMs: 100,
                    reconnectJitterInMs: 50,
                    pingIntervalInMs: 1000,
                    maxPingsOut: 2
                ),

                new NatsConfig("nats://localhost:4222"),

                NatsConfig.CreateWithDefaults("nats://localhost:4222")
            };

            foreach (var config in configs)
            {
                Assert.NotNull(config.endpoint);
                Assert.True(config.connectionTimeoutMs > 0);
                Assert.True(config.requestTimeoutMs > 0);
                Assert.True(config.serverShutdownDeadlineMs > 0);
                Assert.True(config.serverMaxNumberOfRpcs > 0);
                Assert.True(config.maxPendingMessages > 0);
                Assert.True(config.reconnectBufSize > 0);
                Assert.True(config.maxReconnectionAttempts > 0);
                Assert.True(config.reconnectWaitInMs > 0);
                Assert.True(config.reconnectJitterInMs >= 0);
                Assert.True(config.pingIntervalInMs > 0);
                Assert.True(config.maxPingsOut > 0);
            }
        }

        [Fact]
        public void Can_Initialize_With_Old_Constructor_Format()
        {
            var natsConfig = new NatsConfig(
                endpoint: "nats://localhost:4222",
                connectionTimeoutMs: 1000,
                requestTimeoutMs: 5000,
                serverShutdownDeadlineMs: 10 * 1000,
                serverMaxNumberOfRpcs: int.MaxValue,
                maxConnectionRetries: 3, // Deprecated parameter
                maxPendingMessages: 100,
                reconnectBufSize: 4 * 1024 * 1024,
                maxReconnectionAttempts: 3,
                reconnectWaitInMs: 1000,
                reconnectJitterInMs: 100,
                pingIntervalInMs: 1000,
                maxPingsOut: 2
            );

            var sdCfg = new SDConfig(
                endpoints: "127.0.0.1:123123123", // Invalid endpoint to force failure
                etcdPrefix: "pitaya/",
                serverTypeFilters: new List<string>(),
                heartbeatTTLSec: 10,
                logHeartbeat: false,
                logServerSync: false,
                logServerDetails: false,
                syncServersIntervalSec: 10,
                maxNumberOfRetries: 0,
                retryDelayMilliseconds: 0
            );

            var server = new Server(
                id: "id",
                type: "type",
                metadata: "",
                hostname: "",
                frontend: false
            );

            // Should fail initialization (expected due to invalid etcd endpoint)
            // but the important thing is that it doesn't crash due to the deprecated parameter
            Assert.Throws<PitayaException>(() =>
            {
                PitayaCluster.Initialize(natsConfig, sdCfg, server, NativeLogLevel.Debug);
            });

            PitayaCluster.Terminate();
        }

        [Fact]
        public void Can_Initialize_With_New_Constructor_Format()
        {
            var natsConfig = new NatsConfig(
                endpoint: "nats://localhost:4222",
                connectionTimeoutMs: 1000,
                requestTimeoutMs: 5000,
                serverShutdownDeadlineMs: 10 * 1000,
                serverMaxNumberOfRpcs: int.MaxValue,
                maxPendingMessages: 100,
                reconnectBufSize: 4 * 1024 * 1024,
                maxReconnectionAttempts: 3,
                reconnectWaitInMs: 1000,
                reconnectJitterInMs: 100,
                pingIntervalInMs: 1000,
                maxPingsOut: 2
            );

            var sdCfg = new SDConfig(
                endpoints: "127.0.0.1:123123123", // Invalid endpoint to force failure
                etcdPrefix: "pitaya/",
                serverTypeFilters: new List<string>(),
                heartbeatTTLSec: 10,
                logHeartbeat: false,
                logServerSync: false,
                logServerDetails: false,
                syncServersIntervalSec: 10,
                maxNumberOfRetries: 0,
                retryDelayMilliseconds: 0
            );

            var server = new Server(
                id: "id",
                type: "type",
                metadata: "",
                hostname: "",
                frontend: false
            );

            // Should fail initialization (expected due to invalid etcd endpoint)
            // but the important thing is that it doesn't crash due to missing deprecated parameter
            Assert.Throws<PitayaException>(() =>
            {
                PitayaCluster.Initialize(natsConfig, sdCfg, server, NativeLogLevel.Debug);
            });

            PitayaCluster.Terminate();
        }

        [Fact]
        public void Can_Initialize_With_Simple_Constructor()
        {
            var natsConfig = new NatsConfig("nats://localhost:4222");

            var sdCfg = new SDConfig(
                endpoints: "127.0.0.1:123123123", // Invalid endpoint to force failure
                etcdPrefix: "pitaya/",
                serverTypeFilters: new List<string>(),
                heartbeatTTLSec: 10,
                logHeartbeat: false,
                logServerSync: false,
                logServerDetails: false,
                syncServersIntervalSec: 10,
                maxNumberOfRetries: 0,
                retryDelayMilliseconds: 0
            );

            var server = new Server(
                id: "id",
                type: "type",
                metadata: "",
                hostname: "",
                frontend: false
            );

            // Should fail initialization (expected due to invalid etcd endpoint)
            // but the important thing is that it doesn't crash with simple constructor
            Assert.Throws<PitayaException>(() =>
            {
                PitayaCluster.Initialize(natsConfig, sdCfg, server, NativeLogLevel.Debug);
            });

            PitayaCluster.Terminate();
        }
    }
}