#include "common.h"
#include <csignal>
#include <random>

class IntegrationTestClient {
private:
    std::unique_ptr<pitaya::Cluster> cluster_;
    TestConfig config_;
    TestStats stats_;
    std::atomic<bool> running_{true};
    std::atomic<bool> lameDuckMode_{false};
    std::mt19937 rng_{std::random_device{}()};
    
public:
    IntegrationTestClient(const TestConfig& config) : config_(config) {
        SetupSignalHandlers();
    }
    
    ~IntegrationTestClient() {
        Shutdown();
    }
    
    bool Initialize() {
        std::cout << "[CLIENT] " << GetCurrentTimeString() << " Initializing client..." << std::endl;
        
        try {
            // Create NATS and etcd configs
            auto natsConfig = CreateNatsConfig(config_);
            auto etcdConfig = CreateEtcdConfig(config_);
            
            // Initialize cluster
            cluster_ = std::make_unique<pitaya::Cluster>(
                etcdConfig,
                natsConfig,
                "test-client",
                "client",
                pitaya::NatsApiType::Asynchronous
            );
            
            std::cout << "[CLIENT] " << GetCurrentTimeString() << " Client initialized successfully" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "[CLIENT] " << GetCurrentTimeString() 
                      << " Failed to initialize client: " << e.what() << std::endl;
            return false;
        }
    }
    
    void RunLoadTest(int durationSeconds = 60) {
        std::cout << "[CLIENT] " << GetCurrentTimeString() 
                  << " Starting load test for " << durationSeconds << " seconds..." << std::endl;
        
        auto startTime = std::chrono::steady_clock::now();
        auto endTime = startTime + std::chrono::seconds(durationSeconds);
        
        // Start RPC request thread
        std::thread rpcThread([this, endTime]() {
            RunRpcRequests(endTime);
        });
        
        // Start event publishing thread
        std::thread eventThread([this, endTime]() {
            RunEventPublishing(endTime);
        });
        
        // Monitor lame duck mode
        std::thread monitorThread([this, endTime]() {
            MonitorLameDuckMode(endTime);
        });
        
        // Print periodic stats
        std::thread statsThread([this, endTime]() {
            PrintPeriodicStats(endTime);
        });
        
        // Wait for completion
        rpcThread.join();
        eventThread.join();
        monitorThread.join();
        statsThread.join();
        
        std::cout << "[CLIENT] " << GetCurrentTimeString() << " Load test completed" << std::endl;
        PrintStats(stats_, "[CLIENT] ");
    }
    
private:
    void SetupSignalHandlers() {
        std::signal(SIGINT, [](int) {
            std::cout << std::endl << "[CLIENT] Received SIGINT, shutting down gracefully..." << std::endl;
            std::exit(0);
        });
    }
    
    void RunRpcRequests(std::chrono::steady_clock::time_point endTime) {
        std::uniform_int_distribution<int> delayDist(50, 200); // 50-200ms between requests
        int requestId = 1;
        
        while (std::chrono::steady_clock::now() < endTime && running_) {
            try {
                // Create test request
                TestRequest req;
                req.requestId = requestId++;
                req.message = "Hello from client " + std::to_string(req.requestId);
                req.timestamp = std::chrono::steady_clock::now();
                
                // Serialize request (simple JSON-like format)
                std::string requestData = "{\"requestId\":" + std::to_string(req.requestId) + 
                                         ",\"message\":\"" + req.message + "\"}";
                std::vector<uint8_t> data(requestData.begin(), requestData.end());
                
                stats_.totalRequests++;
                
                // Make RPC request
                auto response = cluster_->Request("connector.testhandler.echo", data, config_.requestTimeout);
                
                if (response.GetCode() == pitaya::constants::kCodeOk) {
                    stats_.successfulRequests++;
                    
                    // Log successful request periodically
                    if (req.requestId % 100 == 0) {
                        std::cout << "[CLIENT] " << GetCurrentTimeString() 
                                  << " RPC request " << req.requestId << " successful" << std::endl;
                    }
                } else {
                    stats_.failedRequests++;
                    
                    // Always log failures
                    std::cout << "[CLIENT] " << GetCurrentTimeString() 
                              << " RPC request " << req.requestId << " failed: " 
                              << response.GetError() << " (code: " << response.GetCode() << ")" << std::endl;
                }
                
            } catch (const pitaya::PitayaException& e) {
                stats_.failedRequests++;
                std::cout << "[CLIENT] " << GetCurrentTimeString() 
                          << " RPC request " << requestId << " exception: " << e.what() << std::endl;
            }
            
            // Random delay between requests
            std::this_thread::sleep_for(std::chrono::milliseconds(delayDist(rng_)));
        }
        
        std::cout << "[CLIENT] " << GetCurrentTimeString() << " RPC request thread finished" << std::endl;
    }
    
    void RunEventPublishing(std::chrono::steady_clock::time_point endTime) {
        std::uniform_int_distribution<int> delayDist(100, 500); // 100-500ms between events
        int eventId = 1;
        
        while (std::chrono::steady_clock::now() < endTime && running_) {
            try {
                // Create test event
                std::string eventData = "{\"eventId\":" + std::to_string(eventId++) + 
                                       ",\"timestamp\":\"" + GetCurrentTimeString() + 
                                       "\",\"data\":\"test event from client\"}";
                std::vector<uint8_t> data(eventData.begin(), eventData.end());
                
                stats_.totalEvents++;
                
                // Publish event (using direct NATS client)
                auto natsClient = cluster_->GetNatsClient();
                if (natsClient) {
                    natsStatus status = natsClient->Publish("events.client.test", data);
                    
                    if (status == NATS_OK) {
                        stats_.successfulEvents++;
                        
                        // Log successful event periodically
                        if (eventId % 50 == 0) {
                            std::cout << "[CLIENT] " << GetCurrentTimeString() 
                                      << " Event " << eventId << " published successfully" << std::endl;
                        }
                    } else {
                        stats_.failedEvents++;
                        std::cout << "[CLIENT] " << GetCurrentTimeString() 
                                  << " Event " << eventId << " failed to publish: " << status << std::endl;
                    }
                }
                
            } catch (const std::exception& e) {
                stats_.failedEvents++;
                std::cout << "[CLIENT] " << GetCurrentTimeString() 
                          << " Event " << eventId << " exception: " << e.what() << std::endl;
            }
            
            // Random delay between events
            std::this_thread::sleep_for(std::chrono::milliseconds(delayDist(rng_)));
        }
        
        std::cout << "[CLIENT] " << GetCurrentTimeString() << " Event publishing thread finished" << std::endl;
    }
    
    void MonitorLameDuckMode(std::chrono::steady_clock::time_point endTime) {
        while (std::chrono::steady_clock::now() < endTime && running_) {
            try {
                auto natsClient = cluster_->GetNatsClient();
                if (natsClient) {
                    bool inLameDuckMode = natsClient->IsInLameDuckMode();
                    bool hotSwapAvailable = natsClient->IsHotSwapAvailable();
                    
                    if (inLameDuckMode && !lameDuckMode_.load()) {
                        lameDuckMode_ = true;
                        stats_.lameDuckDetected = true;
                        std::cout << "[CLIENT] " << GetCurrentTimeString() 
                                  << " ⚠️  LAME DUCK MODE DETECTED!" << std::endl;
                    }
                    
                    if (hotSwapAvailable) {
                        stats_.hotSwapActivated = true;
                        std::cout << "[CLIENT] " << GetCurrentTimeString() 
                                  << " 🔄 Hot-swap client activated" << std::endl;
                    }
                    
                    if (!inLameDuckMode && lameDuckMode_.load()) {
                        lameDuckMode_ = false;
                        std::cout << "[CLIENT] " << GetCurrentTimeString() 
                                  << " ✅ Lame duck mode resolved" << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                // Ignore monitoring errors
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        std::cout << "[CLIENT] " << GetCurrentTimeString() << " Lame duck monitoring thread finished" << std::endl;
    }
    
    void PrintPeriodicStats(std::chrono::steady_clock::time_point endTime) {
        auto lastPrint = std::chrono::steady_clock::now();
        
        while (std::chrono::steady_clock::now() < endTime && running_) {
            auto now = std::chrono::steady_clock::now();
            
            if (now - lastPrint >= std::chrono::seconds(10)) {
                std::cout << "[CLIENT] " << GetCurrentTimeString() << " Periodic stats:" << std::endl;
                PrintStats(stats_, "[CLIENT]   ");
                lastPrint = now;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    void Shutdown() {
        running_ = false;
        
        if (cluster_) {
            std::cout << "[CLIENT] " << GetCurrentTimeString() << " Shutting down cluster..." << std::endl;
            cluster_->Terminate();
            cluster_.reset();
        }
    }
};

int main(int argc, char* argv[]) {
    std::cout << "[CLIENT] " << GetCurrentTimeString() << " Starting Pitaya Integration Test Client" << std::endl;
    
    TestConfig config;
    
    // Parse command line arguments
    int testDuration = 60; // default 60 seconds
    if (argc > 1) {
        testDuration = std::atoi(argv[1]);
        if (testDuration <= 0) testDuration = 60;
    }
    
    if (argc > 2) {
        config.natsAddr = argv[2];
    }
    
    std::cout << "[CLIENT] Configuration:" << std::endl;
    std::cout << "[CLIENT]   NATS Address: " << config.natsAddr << std::endl;
    std::cout << "[CLIENT]   Test Duration: " << testDuration << " seconds" << std::endl;
    std::cout << "[CLIENT]   Reconnect Buffer Size: " << config.reconnectBufSize << " bytes" << std::endl;
    
    IntegrationTestClient client(config);
    
    if (!client.Initialize()) {
        std::cerr << "[CLIENT] Failed to initialize client" << std::endl;
        return 1;
    }
    
    // Run the load test
    client.RunLoadTest(testDuration);
    
    std::cout << "[CLIENT] " << GetCurrentTimeString() << " Client finished" << std::endl;
    return 0;
}