#include "common.h"
#include <csignal>

class IntegrationTestServer {
private:
    std::unique_ptr<pitaya::Cluster> cluster_;
    TestConfig config_;
    TestStats stats_;
    std::atomic<bool> running_{true};
    std::atomic<bool> lameDuckMode_{false};
    
public:
    IntegrationTestServer(const TestConfig& config) : config_(config) {
        SetupSignalHandlers();
    }
    
    ~IntegrationTestServer() {
        Shutdown();
    }
    
    bool Initialize() {
        std::cout << "[SERVER] " << GetCurrentTimeString() << " Initializing server..." << std::endl;
        
        try {
            // Create NATS and etcd configs
            auto natsConfig = CreateNatsConfig(config_);
            auto etcdConfig = CreateEtcdConfig(config_);
            
            // Initialize cluster
            cluster_ = std::make_unique<pitaya::Cluster>(
                etcdConfig,
                natsConfig,
                config_.serverId,
                config_.serverType,
                pitaya::NatsApiType::Asynchronous
            );
            
            // Register RPC handlers
            RegisterHandlers();
            
            // Start event consumption
            StartEventConsumer();
            
            std::cout << "[SERVER] " << GetCurrentTimeString() << " Server initialized successfully" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "[SERVER] " << GetCurrentTimeString() 
                      << " Failed to initialize server: " << e.what() << std::endl;
            return false;
        }
    }
    
    void Run() {
        std::cout << "[SERVER] " << GetCurrentTimeString() << " Server running... Press Ctrl+C to stop" << std::endl;
        
        // Start monitoring thread
        std::thread monitorThread([this]() {
            MonitorLameDuckMode();
        });
        
        // Start stats thread
        std::thread statsThread([this]() {
            PrintPeriodicStats();
        });
        
        // Wait for shutdown signal
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        
        monitorThread.join();
        statsThread.join();
        
        PrintStats(stats_, "[SERVER] ");
        std::cout << "[SERVER] " << GetCurrentTimeString() << " Server stopped" << std::endl;
    }
    
private:
    void SetupSignalHandlers() {
        std::signal(SIGINT, [](int) {
            std::cout << std::endl << "[SERVER] Received SIGINT, shutting down gracefully..." << std::endl;
            std::exit(0);
        });
    }
    
    void RegisterHandlers() {
        std::cout << "[SERVER] " << GetCurrentTimeString() << " Registering RPC handlers..." << std::endl;
        
        // Register echo handler
        auto echoHandler = [this](pitaya::Context ctx) -> pitaya::Response {
            stats_.totalRequests++;
            
            try {
                // Parse request
                auto requestData = ctx.GetData();
                std::string requestStr(requestData.begin(), requestData.end());
                
                // Log request periodically
                static int requestCount = 0;
                requestCount++;
                if (requestCount % 100 == 0) {
                    std::cout << "[SERVER] " << GetCurrentTimeString() 
                              << " Processing RPC request " << requestCount << std::endl;
                }
                
                // Create response
                std::string responseData = "{\"response\":\"Echo: " + requestStr + "\",\"timestamp\":\"" + 
                                         GetCurrentTimeString() + "\",\"server\":\"" + config_.serverId + "\"}";
                std::vector<uint8_t> responseBytes(responseData.begin(), responseData.end());
                
                stats_.successfulRequests++;
                return pitaya::Response::New(responseBytes);
                
            } catch (const std::exception& e) {
                stats_.failedRequests++;
                std::cout << "[SERVER] " << GetCurrentTimeString() 
                          << " Error processing request: " << e.what() << std::endl;
                return pitaya::Response::Error("Internal server error");
            }
        };
        
        cluster_->RegisterRPCJob("testhandler", "echo", echoHandler);
        std::cout << "[SERVER] " << GetCurrentTimeString() << " RPC handlers registered" << std::endl;
    }
    
    void StartEventConsumer() {
        std::cout << "[SERVER] " << GetCurrentTimeString() << " Starting event consumer..." << std::endl;
        
        try {
            auto natsClient = cluster_->GetNatsClient();
            if (natsClient) {
                auto eventHandler = [this](std::shared_ptr<pitaya::NatsMsg> msg) {
                    stats_.totalEvents++;
                    
                    try {
                        // Process event
                        auto data = msg->GetData();
                        size_t size = msg->GetSize();
                        std::string eventStr(data, data + size);
                        
                        // Log event periodically
                        static int eventCount = 0;
                        eventCount++;
                        if (eventCount % 50 == 0) {
                            std::cout << "[SERVER] " << GetCurrentTimeString() 
                                      << " Processed event " << eventCount << std::endl;
                        }
                        
                        stats_.successfulEvents++;
                        
                    } catch (const std::exception& e) {
                        stats_.failedEvents++;
                        std::cout << "[SERVER] " << GetCurrentTimeString() 
                                  << " Error processing event: " << e.what() << std::endl;
                    }
                };
                
                natsStatus status = natsClient->Subscribe("events.client.test", eventHandler);
                if (status == NATS_OK) {
                    std::cout << "[SERVER] " << GetCurrentTimeString() 
                              << " Event consumer started successfully" << std::endl;
                } else {
                    std::cout << "[SERVER] " << GetCurrentTimeString() 
                              << " Failed to start event consumer: " << status << std::endl;
                }
            }
            
        } catch (const std::exception& e) {
            std::cout << "[SERVER] " << GetCurrentTimeString() 
                      << " Error starting event consumer: " << e.what() << std::endl;
        }
    }
    
    void MonitorLameDuckMode() {
        while (running_) {
            try {
                auto natsClient = cluster_->GetNatsClient();
                if (natsClient) {
                    bool inLameDuckMode = natsClient->IsInLameDuckMode();
                    bool hotSwapAvailable = natsClient->IsHotSwapAvailable();
                    
                    if (inLameDuckMode && !lameDuckMode_.load()) {
                        lameDuckMode_ = true;
                        stats_.lameDuckDetected = true;
                        std::cout << "[SERVER] " << GetCurrentTimeString() 
                                  << " ⚠️  LAME DUCK MODE DETECTED!" << std::endl;
                    }
                    
                    if (hotSwapAvailable) {
                        stats_.hotSwapActivated = true;
                        std::cout << "[SERVER] " << GetCurrentTimeString() 
                                  << " 🔄 Hot-swap client activated" << std::endl;
                    }
                    
                    if (!inLameDuckMode && lameDuckMode_.load()) {
                        lameDuckMode_ = false;
                        std::cout << "[SERVER] " << GetCurrentTimeString() 
                                  << " ✅ Lame duck mode resolved" << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                // Ignore monitoring errors
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        
        std::cout << "[SERVER] " << GetCurrentTimeString() << " Lame duck monitoring thread finished" << std::endl;
    }
    
    void PrintPeriodicStats() {
        auto lastPrint = std::chrono::steady_clock::now();
        
        while (running_) {
            auto now = std::chrono::steady_clock::now();
            
            if (now - lastPrint >= std::chrono::seconds(15)) {
                std::cout << "[SERVER] " << GetCurrentTimeString() << " Periodic stats:" << std::endl;
                PrintStats(stats_, "[SERVER]   ");
                lastPrint = now;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    void Shutdown() {
        running_ = false;
        
        if (cluster_) {
            std::cout << "[SERVER] " << GetCurrentTimeString() << " Shutting down cluster..." << std::endl;
            cluster_->Terminate();
            cluster_.reset();
        }
    }
};

int main(int argc, char* argv[]) {
    std::cout << "[SERVER] " << GetCurrentTimeString() << " Starting Pitaya Integration Test Server" << std::endl;
    
    TestConfig config;
    
    // Parse command line arguments
    if (argc > 1) {
        config.serverId = argv[1];
    }
    
    if (argc > 2) {
        config.natsAddr = argv[2];
    }
    
    if (argc > 3) {
        config.etcdAddr = argv[3];
    }
    
    std::cout << "[SERVER] Configuration:" << std::endl;
    std::cout << "[SERVER]   Server ID: " << config.serverId << std::endl;
    std::cout << "[SERVER]   Server Type: " << config.serverType << std::endl;
    std::cout << "[SERVER]   NATS Address: " << config.natsAddr << std::endl;
    std::cout << "[SERVER]   etcd Address: " << config.etcdAddr << std::endl;
    std::cout << "[SERVER]   Reconnect Buffer Size: " << config.reconnectBufSize << " bytes" << std::endl;
    
    IntegrationTestServer server(config);
    
    if (!server.Initialize()) {
        std::cerr << "[SERVER] Failed to initialize server" << std::endl;
        return 1;
    }
    
    // Run the server
    server.Run();
    
    std::cout << "[SERVER] " << GetCurrentTimeString() << " Server finished" << std::endl;
    return 0;
}