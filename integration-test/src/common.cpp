#include "common.h"
#include <iomanip>
#include <sstream>

pitaya::NatsConfig CreateNatsConfig(const TestConfig& config) {
    pitaya::NatsConfig natsConfig;
    natsConfig.natsAddr = config.natsAddr;
    natsConfig.connectionTimeout = config.connectionTimeout;
    natsConfig.requestTimeout = config.requestTimeout;
    natsConfig.maxReconnectionAttempts = config.maxReconnectionAttempts;
    natsConfig.reconnectBufSize = config.reconnectBufSize;
    natsConfig.reconnectWaitInMs = config.reconnectWaitInMs;
    natsConfig.reconnectJitterInMs = config.reconnectJitterInMs;
    natsConfig.pingIntervalInMs = config.pingIntervalInMs;
    natsConfig.maxPingsOut = config.maxPingsOut;
    natsConfig.maxPendingMsgs = config.maxPendingMsgs;
    natsConfig.drainTimeout = config.drainTimeout;
    natsConfig.flushTimeout = config.flushTimeout;
    return natsConfig;
}

pitaya::EtcdConfig CreateEtcdConfig(const TestConfig& config) {
    pitaya::EtcdConfig etcdConfig;
    etcdConfig.endpoint = config.etcdAddr;
    etcdConfig.username = "";
    etcdConfig.password = "";
    etcdConfig.prefix = "pitaya/";
    etcdConfig.serverTypeFilters = {config.serverType};
    etcdConfig.dialTimeoutMs = 5000;
    etcdConfig.etcdRequestTimeoutMs = 3000;
    etcdConfig.heartbeatTTLSec = 30;
    etcdConfig.heartbeatIntervalSec = 10;
    etcdConfig.autoSyncIntervalSec = 30;
    etcdConfig.syncTimeoutSec = 5;
    etcdConfig.logHeartbeat = false;
    etcdConfig.logServerSync = false;
    etcdConfig.logServerDiscovery = false;
    return etcdConfig;
}

void PrintStats(const TestStats& stats, const std::string& prefix) {
    std::cout << prefix << "=== Integration Test Statistics ===" << std::endl;
    std::cout << prefix << "RPC Requests:" << std::endl;
    std::cout << prefix << "  Total: " << stats.totalRequests.load() << std::endl;
    std::cout << prefix << "  Successful: " << stats.successfulRequests.load() << std::endl;
    std::cout << prefix << "  Failed: " << stats.failedRequests.load() << std::endl;
    std::cout << prefix << "  Timeout: " << stats.timeoutRequests.load() << std::endl;
    
    std::cout << prefix << "Events:" << std::endl;
    std::cout << prefix << "  Total: " << stats.totalEvents.load() << std::endl;
    std::cout << prefix << "  Successful: " << stats.successfulEvents.load() << std::endl;
    std::cout << prefix << "  Failed: " << stats.failedEvents.load() << std::endl;
    
    std::cout << prefix << "Lame Duck Mode:" << std::endl;
    std::cout << prefix << "  Detected: " << (stats.lameDuckDetected.load() ? "Yes" : "No") << std::endl;
    std::cout << prefix << "  Hot-swap Activated: " << (stats.hotSwapActivated.load() ? "Yes" : "No") << std::endl;
    
    // Calculate success rates
    int totalRpc = stats.totalRequests.load();
    int totalEvt = stats.totalEvents.load();
    
    if (totalRpc > 0) {
        double rpcSuccessRate = (double)stats.successfulRequests.load() / totalRpc * 100.0;
        std::cout << prefix << "  RPC Success Rate: " << std::fixed << std::setprecision(2) 
                  << rpcSuccessRate << "%" << std::endl;
    }
    
    if (totalEvt > 0) {
        double eventSuccessRate = (double)stats.successfulEvents.load() / totalEvt * 100.0;
        std::cout << prefix << "  Event Success Rate: " << std::fixed << std::setprecision(2) 
                  << eventSuccessRate << "%" << std::endl;
    }
    
    std::cout << prefix << "===================================" << std::endl;
}

std::string GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}