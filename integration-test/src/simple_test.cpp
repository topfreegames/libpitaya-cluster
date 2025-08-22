#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <random>
#include <csignal>
#include <iomanip>

// Use NATS C client directly - much simpler
#include <nats/nats.h>

class SimpleNatsTest {
private:
    natsConnection* conn_;
    natsSubscription* sub_;
    std::atomic<bool> running_;
    std::atomic<int> totalRequests_;
    std::atomic<int> successfulRequests_;
    std::atomic<int> failedRequests_;
    std::atomic<int> totalEvents_;
    std::atomic<int> successfulEvents_;
    std::atomic<int> failedEvents_;
    std::atomic<bool> lameDuckDetected_;
    std::string serverUrls_;
    
public:
    SimpleNatsTest(const std::string& urls) : 
        conn_(nullptr), sub_(nullptr), running_(true),
        totalRequests_(0), successfulRequests_(0), failedRequests_(0),
        totalEvents_(0), successfulEvents_(0), failedEvents_(0),
        lameDuckDetected_(false), serverUrls_(urls) {
        
        // Setup signal handler
        std::signal(SIGINT, [](int) {
            std::cout << "\nReceived SIGINT, shutting down..." << std::endl;
            std::exit(0);
        });
    }
    
    ~SimpleNatsTest() {
        Cleanup();
    }
    
    bool Connect() {
        natsStatus status;
        natsOptions* opts = nullptr;
        
        // Create connection options
        status = natsOptions_Create(&opts);
        if (status != NATS_OK) {
            std::cerr << "Failed to create options: " << natsStatus_GetText(status) << std::endl;
            return false;
        }
        
        // Configure reconnect options
        natsOptions_SetMaxReconnect(opts, 10);
        natsOptions_SetReconnectWait(opts, 2000); // 2s
        natsOptions_SetReconnectJitter(opts, 100, 100); // 100ms jitter
        natsOptions_SetPingInterval(opts, 2000); // 2s ping interval
        natsOptions_SetMaxPingsOut(opts, 2);
        natsOptions_SetReconnectBufSize(opts, 8 * 1024 * 1024); // 8MB buffer
        
        // Set lame duck mode callback
        natsOptions_SetLameDuckModeCB(opts, LameDuckCallback, this);
        natsOptions_SetReconnectedCB(opts, ReconnectedCallback, this);
        natsOptions_SetDisconnectedCB(opts, DisconnectedCallback, this);
        
        // Set the URL in options
        natsOptions_SetURL(opts, serverUrls_.c_str());
        
        // Connect - CORRECT: uses opts with callbacks
        status = natsConnection_Connect(&conn_, opts);
        if (status != NATS_OK) {
            std::cerr << "Failed to connect: " << natsStatus_GetText(status) << std::endl;
            natsOptions_Destroy(opts);
            return false;
        }
        
        std::cout << "Connected to NATS at " << serverUrls_ << std::endl;
        natsOptions_Destroy(opts);
        return true;
    }
    
    void RunLoadTest(int durationSeconds) {
        std::cout << "Starting load test for " << durationSeconds << " seconds..." << std::endl;
        
        // Start subscriber for events
        StartEventSubscriber();
        
        // Start threads
        std::thread rpcThread([this, durationSeconds]() {
            RunRequestReplyLoad(durationSeconds);
        });
        
        std::thread eventThread([this, durationSeconds]() {
            RunEventPublishing(durationSeconds);
        });
        
        std::thread statsThread([this, durationSeconds]() {
            PrintPeriodicStats(durationSeconds);
        });
        
        // Wait for completion
        rpcThread.join();
        eventThread.join();
        statsThread.join();
        
        PrintFinalStats();
    }
    
private:
    static void LameDuckCallback(natsConnection* nc, void* closure) {
        SimpleNatsTest* self = static_cast<SimpleNatsTest*>(closure);
        self->lameDuckDetected_ = true;
        std::cout << "⚠️  LAME DUCK MODE DETECTED!" << std::endl;
        
        // In a real implementation, we would activate hot-swap client here
        std::cout << "🔄 Would activate hot-swap client here" << std::endl;
    }
    
    static void ReconnectedCallback(natsConnection* nc, void* closure) {
        SimpleNatsTest* self = static_cast<SimpleNatsTest*>(closure);
        std::cout << "✅ Reconnected to NATS server" << std::endl;
    }
    
    static void DisconnectedCallback(natsConnection* nc, void* closure) {
        SimpleNatsTest* self = static_cast<SimpleNatsTest*>(closure);
        std::cout << "🔌 Disconnected from NATS server" << std::endl;
    }
    
    static void EventMessageCallback(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure) {
        SimpleNatsTest* self = static_cast<SimpleNatsTest*>(closure);
        
        // Process the event message
        const char* data = natsMsg_GetData(msg);
        int len = natsMsg_GetDataLength(msg);
        
        self->successfulEvents_++;
        
        // Log periodically
        static int eventCount = 0;
        eventCount++;
        if (eventCount % 50 == 0) {
            std::cout << "📨 Processed " << eventCount << " events" << std::endl;
        }
        
        natsMsg_Destroy(msg);
    }
    
    void StartEventSubscriber() {
        natsStatus status = natsConnection_Subscribe(&sub_, conn_, "test.events", 
                                                   EventMessageCallback, this);
        if (status != NATS_OK) {
            std::cerr << "Failed to subscribe: " << natsStatus_GetText(status) << std::endl;
        } else {
            std::cout << "📥 Subscribed to test.events" << std::endl;
        }
    }
    
    void RunRequestReplyLoad(int durationSeconds) {
        auto endTime = std::chrono::steady_clock::now() + std::chrono::seconds(durationSeconds);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> delayDist(50, 200);
        
        int requestId = 1;
        
        while (std::chrono::steady_clock::now() < endTime && running_) {
            totalRequests_++;
            
            // Create request message
            std::string requestData = "Request " + std::to_string(requestId) + 
                                    " at " + GetCurrentTimeString();
            
            natsMsg* reply = nullptr;
            natsStatus status = natsConnection_RequestString(&reply, conn_, 
                                                           "test.rpc", requestData.c_str(), 5000);
            
            if (status == NATS_OK) {
                successfulRequests_++;
                
                if (requestId % 100 == 0) {
                    std::cout << "✅ RPC " << requestId << " successful" << std::endl;
                }
                
                natsMsg_Destroy(reply);
            } else {
                failedRequests_++;
                std::cout << "❌ RPC " << requestId << " failed: " 
                          << natsStatus_GetText(status) << std::endl;
            }
            
            requestId++;
            
            // Random delay
            std::this_thread::sleep_for(std::chrono::milliseconds(delayDist(gen)));
        }
        
        std::cout << "🏁 RPC thread finished" << std::endl;
    }
    
    void RunEventPublishing(int durationSeconds) {
        auto endTime = std::chrono::steady_clock::now() + std::chrono::seconds(durationSeconds);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> delayDist(100, 500);
        
        int eventId = 1;
        
        while (std::chrono::steady_clock::now() < endTime && running_) {
            totalEvents_++;
            
            // Create event message
            std::string eventData = "Event " + std::to_string(eventId) + 
                                  " at " + GetCurrentTimeString();
            
            natsStatus status = natsConnection_PublishString(conn_, "test.events", 
                                                           eventData.c_str());
            
            if (status == NATS_OK) {
                successfulEvents_++;
                
                if (eventId % 50 == 0) {
                    std::cout << "📤 Event " << eventId << " published" << std::endl;
                }
            } else {
                failedEvents_++;
                std::cout << "❌ Event " << eventId << " failed: " 
                          << natsStatus_GetText(status) << std::endl;
            }
            
            eventId++;
            
            // Random delay
            std::this_thread::sleep_for(std::chrono::milliseconds(delayDist(gen)));
        }
        
        std::cout << "🏁 Event thread finished" << std::endl;
    }
    
    void PrintPeriodicStats(int durationSeconds) {
        auto startTime = std::chrono::steady_clock::now();
        auto endTime = startTime + std::chrono::seconds(durationSeconds);
        
        while (std::chrono::steady_clock::now() < endTime && running_) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime).count();
                
            std::cout << "\n📊 Stats at " << elapsed << "s:" << std::endl;
            std::cout << "   RPC: " << successfulRequests_ << "/" << totalRequests_ 
                      << " (" << GetSuccessRate(successfulRequests_, totalRequests_) << "%)" << std::endl;
            std::cout << "   Events: " << successfulEvents_ << "/" << totalEvents_ 
                      << " (" << GetSuccessRate(successfulEvents_, totalEvents_) << "%)" << std::endl;
            std::cout << "   Lame Duck: " << (lameDuckDetected_ ? "YES" : "NO") << std::endl;
        }
    }
    
    void PrintFinalStats() {
        std::cout << "\n🎯 FINAL TEST RESULTS:" << std::endl;
        std::cout << "===============================" << std::endl;
        std::cout << "RPC Operations:" << std::endl;
        std::cout << "  Total: " << totalRequests_ << std::endl;
        std::cout << "  Successful: " << successfulRequests_ << std::endl;
        std::cout << "  Failed: " << failedRequests_ << std::endl;
        std::cout << "  Success Rate: " << GetSuccessRate(successfulRequests_, totalRequests_) << "%" << std::endl;
        
        std::cout << "\nEvent Operations:" << std::endl;
        std::cout << "  Total: " << totalEvents_ << std::endl;
        std::cout << "  Successful: " << successfulEvents_ << std::endl;
        std::cout << "  Failed: " << failedEvents_ << std::endl;
        std::cout << "  Success Rate: " << GetSuccessRate(successfulEvents_, totalEvents_) << "%" << std::endl;
        
        std::cout << "\nLame Duck Detection: " << (lameDuckDetected_ ? "✅ YES" : "❌ NO") << std::endl;
        
        // Determine test result
        int rpcRate = GetSuccessRate(successfulRequests_, totalRequests_);
        int eventRate = GetSuccessRate(successfulEvents_, totalEvents_);
        
        if (lameDuckDetected_ && rpcRate >= 95 && eventRate >= 95 && 
            failedRequests_ == 0 && failedEvents_ == 0) {
            std::cout << "\n🎉 TEST PASSED - ZERO DOWNTIME ACHIEVED!" << std::endl;
        } else {
            std::cout << "\n❌ TEST FAILED - Requirements not met" << std::endl;
        }
        std::cout << "===============================" << std::endl;
    }
    
    int GetSuccessRate(int successful, int total) {
        return total > 0 ? (successful * 100 / total) : 0;
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
    
    void Cleanup() {
        running_ = false;
        
        if (sub_) {
            natsSubscription_Destroy(sub_);
            sub_ = nullptr;
        }
        
        if (conn_) {
            natsConnection_Close(conn_);
            natsConnection_Destroy(conn_);
            conn_ = nullptr;
        }
        
        nats_Close();
    }
};

int main(int argc, char* argv[]) {
    std::cout << "🚀 Starting Simple NATS Integration Test" << std::endl;
    
    // Parse arguments
    int testDuration = 60;
    std::string natsUrls = "nats://localhost:4222";
    
    if (argc > 1) {
        testDuration = std::atoi(argv[1]);
        if (testDuration <= 0) testDuration = 60;
    }
    
    if (argc > 2) {
        natsUrls = argv[2];
    }
    
    std::cout << "⚙️  Configuration:" << std::endl;
    std::cout << "   Duration: " << testDuration << " seconds" << std::endl;
    std::cout << "   NATS URLs: " << natsUrls << std::endl;
    
    // Initialize NATS library
    natsStatus status = nats_Open(-1);
    if (status != NATS_OK) {
        std::cerr << "Failed to initialize NATS: " << natsStatus_GetText(status) << std::endl;
        return 1;
    }
    
    // Create and run test
    SimpleNatsTest test(natsUrls);
    
    if (!test.Connect()) {
        std::cerr << "❌ Failed to connect to NATS" << std::endl;
        return 1;
    }
    
    test.RunLoadTest(testDuration);
    
    std::cout << "✅ Test completed" << std::endl;
    return 0;
}