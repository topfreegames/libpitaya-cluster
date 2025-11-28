#include "pitaya/c_wrapper.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <signal.h>
#include <cstring>

// Global atomic variable to detect SIGTERM
static std::atomic<bool> gSigtermReceived{false};

// Signal handler to capture SIGTERM
void sigterm_handler(int signum) {
    if (signum == SIGTERM) {
        gSigtermReceived = true;
    }
}

// Resets the test state
void reset_test_state() {
    gSigtermReceived = false;
}

// Installs the signal handler
void install_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
}

// Test 1: No retry (maxReconnectionAttempts = 0) - immediate failure
bool test_sigterm_no_retry() {
    std::cout << "\n=== TEST 1: No retry (immediate failure) ===" << std::endl;
    
    reset_test_state();
    install_signal_handler();
    
    std::thread worker([]() {
        std::cout << "[Thread] Attempting to initialize without retry..." << std::endl;
        
        CNATSConfig nc = {};
        nc.addr = "nats://192.0.2.1:4222";  // Invalid IP
        nc.connectionTimeoutMs = 100;
        nc.requestTimeoutMs = 1000;
        nc.serverShutdownDeadlineMs = 1000;
        nc.serverMaxNumberOfRpcs = 100;
        nc.maxReconnectionAttempts = 0;  // NO RETRY
        nc.maxPendingMsgs = 1000;
        nc.reconnectWaitInMs = 100;
        nc.reconnectBufSize = 8*1024*1024;
        nc.reconnectJitterInMs = 50;
        nc.pingIntervalInMs = 1000;
        nc.maxPingsOut = 2;
        
        CSDConfig sd = {};
        sd.endpoints = "http://127.0.0.1:2379";
        sd.etcdPrefix = "pitaya/";
        sd.heartbeatTTLSec = 5;
        sd.logHeartbeat = false;
        sd.logServerSync = false;
        sd.logServerDetails = false;
        sd.syncServersIntervalSec = 20;
        sd.serverTypeFilters = nullptr;
        sd.initializationTimeoutSec = 2;
        
        CServer sv = {};
        sv.id = (char*)"test";
        sv.type = (char*)"test";
        sv.metadata = (char*)"{}";
        sv.hostname = (char*)"localhost";
        sv.frontend = true;
        
        tfg_pitc_InitializeWithNats(&nc, &sd, &sv, LogLevel_Info, nullptr);
        
        std::cout << "[Thread] Initialization returned" << std::endl;
    });
    
    // Wait for the thread to finish
    worker.join();
    
    // Small delay to ensure the signal handler is called
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (gSigtermReceived) {
        std::cout << "✓ TEST 1 PASSED: SIGTERM received by parent process (no retry)" << std::endl;
        return true;
    }
    
    std::cout << "✗ TEST 1 FAILED: SIGTERM was not received" << std::endl;
    return false;
}

// Test 2: With retry (maxReconnectionAttempts > 0) - failure after retries
bool test_sigterm_with_retry() {
    std::cout << "\n=== TEST 2: With retry (failure after retries) ===" << std::endl;
    
    reset_test_state();
    install_signal_handler();
    
    std::atomic<bool> threadFinished{false};
    
    std::thread worker([&threadFinished]() {
        std::cout << "[Thread] Attempting to initialize with 2 retries..." << std::endl;
        
        CNATSConfig nc = {};
        nc.addr = "nats://192.0.2.1:4222";  // Invalid IP
        nc.connectionTimeoutMs = 100;       // 100ms timeout per attempt
        nc.requestTimeoutMs = 1000;
        nc.serverShutdownDeadlineMs = 1000;
        nc.serverMaxNumberOfRpcs = 100;
        nc.maxReconnectionAttempts = 2;     // 2 RETRIES
        nc.maxPendingMsgs = 1000;
        nc.reconnectWaitInMs = 100;         // 100ms between retries
        nc.reconnectBufSize = 8*1024*1024;
        nc.reconnectJitterInMs = 50;
        nc.pingIntervalInMs = 1000;
        nc.maxPingsOut = 2;
        
        CSDConfig sd = {};
        sd.endpoints = "http://127.0.0.1:2379";
        sd.etcdPrefix = "pitaya/";
        sd.heartbeatTTLSec = 5;
        sd.logHeartbeat = false;
        sd.logServerSync = false;
        sd.logServerDetails = false;
        sd.syncServersIntervalSec = 20;
        sd.serverTypeFilters = nullptr;
        sd.initializationTimeoutSec = 10;   // Larger timeout to allow retries
        
        CServer sv = {};
        sv.id = (char*)"test";
        sv.type = (char*)"test";
        sv.metadata = (char*)"{}";
        sv.hostname = (char*)"localhost";
        sv.frontend = true;
        
        tfg_pitc_InitializeWithNats(&nc, &sd, &sv, LogLevel_Info, nullptr);
        
        std::cout << "[Thread] Initialization returned, waiting for SIGTERM from retries..." << std::endl;
        
        // Wait for SIGTERM from ClosedCb when retries fail
        for (int i = 0; i < 100 && !gSigtermReceived; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        threadFinished = true;
    });
    
    // Wait with timeout
    int timeout_sec = 15;
    for (int i = 0; i < timeout_sec * 10; i++) {
        if (gSigtermReceived) {
            worker.join();
            std::cout << "✓ TEST 2 PASSED: SIGTERM received by parent process after retries failed" << std::endl;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Timeout
    worker.join();
    std::cout << "✗ TEST 2 FAILED: Timeout waiting for SIGTERM" << std::endl;
    return false;
}

// Test 3: Successful connection - should NOT send SIGTERM
bool test_successful_connection() {
    std::cout << "\n=== TEST 3: Successful connection (no SIGTERM) ===" << std::endl;
    
    reset_test_state();
    install_signal_handler();
    
    std::atomic<bool> initSuccess{false};
    
    std::thread worker([&initSuccess]() {
        std::cout << "[Thread] Attempting to connect to real NATS and etcd..." << std::endl;
        
        CNATSConfig nc = {};
        nc.addr = "nats://127.0.0.1:4222";  // Local NATS (Docker)
        nc.connectionTimeoutMs = 5000;
        nc.requestTimeoutMs = 5000;
        nc.serverShutdownDeadlineMs = 1000;
        nc.serverMaxNumberOfRpcs = 100;
        nc.maxReconnectionAttempts = 3;
        nc.maxPendingMsgs = 1000;
        nc.reconnectWaitInMs = 100;
        nc.reconnectBufSize = 8*1024*1024;
        nc.reconnectJitterInMs = 50;
        nc.pingIntervalInMs = 1000;
        nc.maxPingsOut = 2;
        
        CSDConfig sd = {};
        sd.endpoints = "http://127.0.0.1:2379";  // Local etcd (Docker)
        sd.etcdPrefix = "pitaya/";
        sd.heartbeatTTLSec = 5;
        sd.logHeartbeat = false;
        sd.logServerSync = false;
        sd.logServerDetails = false;
        sd.syncServersIntervalSec = 20;
        sd.serverTypeFilters = nullptr;
        sd.initializationTimeoutSec = 10;
        
        CServer sv = {};
        sv.id = (char*)"test-server-1";
        sv.type = (char*)"connector";
        sv.metadata = (char*)"{}";
        sv.hostname = (char*)"localhost";
        sv.frontend = true;
        
        bool result = tfg_pitc_InitializeWithNats(&nc, &sd, &sv, LogLevel_Info, nullptr);
        
        if (result) {
            std::cout << "[Thread] ✓ Initialization successful!" << std::endl;
            initSuccess = true;
            
            // Wait a bit to ensure stability
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            // Terminate normally
            tfg_pitc_Terminate();
            std::cout << "[Thread] Terminated normally" << std::endl;
        } else {
            std::cout << "[Thread] ✗ Initialization failed!" << std::endl;
        }
    });
    
    // Wait for thread with timeout
    int timeout_sec = 20;
    for (int i = 0; i < timeout_sec * 10; i++) {
        if (gSigtermReceived) {
            worker.join();
            std::cout << "✗ TEST 3 FAILED: SIGTERM was received (it shouldn't!)" << std::endl;
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // If thread finished successfully
        if (initSuccess) {
            break;
        }
    }
    
    worker.join();
    
    if (initSuccess && !gSigtermReceived) {
        std::cout << "✓ TEST 3 PASSED: Successful connection, no SIGTERM" << std::endl;
        return true;
    }
    
    if (!initSuccess) {
        std::cout << "✗ TEST 3 FAILED: Initialization was not successful" << std::endl;
    }
    
    return false;
}

int main(int argc, char* argv[]) {
    bool run_all = (argc == 1);
    bool run_test1 = run_all || (argc > 1 && strcmp(argv[1], "1") == 0);
    bool run_test2 = run_all || (argc > 1 && strcmp(argv[1], "2") == 0);
    bool run_test3 = run_all || (argc > 1 && strcmp(argv[1], "3") == 0);
    
    int failures = 0;
    
    if (run_test1) {
        if (!test_sigterm_no_retry()) failures++;
    }
    
    if (run_test2) {
        if (!test_sigterm_with_retry()) failures++;
    }
    
    if (run_test3) {
        if (!test_successful_connection()) failures++;
    }
    
    std::cout << "\n=== RESULT ===" << std::endl;
    if (failures == 0) {
        std::cout << "✓ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "✗ " << failures << " test(s) failed" << std::endl;
        return 1;
    }
}
