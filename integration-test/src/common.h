#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>
#include <cstdint>

// Forward declarations to avoid complex protobuf dependencies
namespace pitaya {
    class Cluster;
    class NatsClient;
    struct NatsConfig;
    struct EtcdConfig;
    
    enum class NatsApiType {
        Synchronous,
        Asynchronous
    };
    
    class Response {
    public:
        static Response New(const std::vector<uint8_t>& data);
        static Response Error(const std::string& error);
        int GetCode() const;
        std::string GetError() const;
    };
    
    class Context {
    public:
        std::vector<uint8_t> GetData() const;
    };
    
    namespace constants {
        extern const int kCodeOk;
        extern const int kCodeServiceUnavailable;
    }
    
    class PitayaException : public std::exception {
    public:
        const char* what() const noexcept override;
    };
}

// Define the config structs here to avoid header dependencies
struct NatsConfigSimple {
    std::string natsAddr = "nats://localhost:4222";
    std::chrono::milliseconds connectionTimeout = std::chrono::milliseconds(5000);
    std::chrono::milliseconds requestTimeout = std::chrono::milliseconds(5000);
    int maxReconnectionAttempts = 10;
    int reconnectBufSize = 8 * 1024 * 1024; // 8MB
    std::chrono::milliseconds reconnectWaitInMs = std::chrono::milliseconds(2000);
    std::chrono::milliseconds reconnectJitterInMs = std::chrono::milliseconds(100);
    std::chrono::milliseconds pingIntervalInMs = std::chrono::milliseconds(2000);
    int maxPingsOut = 2;
    int maxPendingMsgs = 10000;
    std::chrono::milliseconds drainTimeout = std::chrono::milliseconds(30000);
    std::chrono::milliseconds flushTimeout = std::chrono::milliseconds(5000);
};

struct EtcdConfigSimple {
    std::string endpoint = "http://localhost:2379";
    std::string username = "";
    std::string password = "";
    std::string prefix = "pitaya/";
    std::vector<std::string> serverTypeFilters;
    int dialTimeoutMs = 5000;
    int etcdRequestTimeoutMs = 3000;
    int heartbeatTTLSec = 30;
    int heartbeatIntervalSec = 10;
    int autoSyncIntervalSec = 30;
    int syncTimeoutSec = 5;
    bool logHeartbeat = false;
    bool logServerSync = false;
    bool logServerDiscovery = false;
};

// Common configuration
struct TestConfig {
    std::string natsAddr = "nats://localhost:4222";
    std::string etcdAddr = "http://localhost:2379";
    std::string serverType = "connector";
    std::string serverId = "test-server";
    std::chrono::milliseconds requestTimeout = std::chrono::milliseconds(5000);
    std::chrono::milliseconds connectionTimeout = std::chrono::milliseconds(5000);
    int maxReconnectionAttempts = 10;
    int reconnectBufSize = 8 * 1024 * 1024; // 8MB for application buffer
    std::chrono::milliseconds reconnectWaitInMs = std::chrono::milliseconds(2000);
    std::chrono::milliseconds reconnectJitterInMs = std::chrono::milliseconds(100);
    std::chrono::milliseconds pingIntervalInMs = std::chrono::milliseconds(2000);
    int maxPingsOut = 2;
    int maxPendingMsgs = 10000;
    std::chrono::milliseconds drainTimeout = std::chrono::milliseconds(30000);
    std::chrono::milliseconds flushTimeout = std::chrono::milliseconds(5000);
};

// Statistics tracking
struct TestStats {
    std::atomic<int> totalRequests{0};
    std::atomic<int> successfulRequests{0};
    std::atomic<int> failedRequests{0};
    std::atomic<int> timeoutRequests{0};
    std::atomic<int> totalEvents{0};
    std::atomic<int> successfulEvents{0};
    std::atomic<int> failedEvents{0};
    std::atomic<bool> lameDuckDetected{false};
    std::atomic<bool> hotSwapActivated{false};
};

// Common utility functions
pitaya::NatsConfig CreateNatsConfig(const TestConfig& config);
pitaya::EtcdConfig CreateEtcdConfig(const TestConfig& config);
void PrintStats(const TestStats& stats, const std::string& prefix = "");
std::string GetCurrentTimeString();

// Test data structures
struct TestRequest {
    std::string message;
    int requestId;
    std::chrono::steady_clock::time_point timestamp;
};

struct TestResponse {
    std::string response;
    int requestId;
    bool success;
};