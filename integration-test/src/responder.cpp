#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <csignal>
#include <iomanip>

// Use NATS C client directly
#include <nats/nats.h>

class NatsResponder {
private:
    natsConnection* conn_;
    natsSubscription* rpcSub_;
    natsSubscription* eventSub_;
    std::atomic<bool> running_;
    std::atomic<int> totalRpcRequests_;
    std::atomic<int> successfulRpcResponses_;
    std::atomic<int> failedRpcResponses_;
    std::atomic<int> totalEvents_;
    std::atomic<int> processedEvents_;
    std::atomic<bool> lameDuckDetected_;
    std::string serverUrls_;
    
public:
    NatsResponder(const std::string& urls) : 
        conn_(nullptr), rpcSub_(nullptr), eventSub_(nullptr), running_(true),
        totalRpcRequests_(0), successfulRpcResponses_(0), failedRpcResponses_(0),
        totalEvents_(0), processedEvents_(0),
        lameDuckDetected_(false), serverUrls_(urls) {
        
        // Setup signal handler
        std::signal(SIGINT, [](int) {
            std::cout << "\n[RESPONDER] Received SIGINT, shutting down..." << std::endl;
            std::exit(0);
        });
    }
    
    ~NatsResponder() {
        Cleanup();
    }
    
    bool Connect() {
        natsStatus status;
        natsOptions* opts = nullptr;
        
        // Create connection options
        status = natsOptions_Create(&opts);
        if (status != NATS_OK) {
            std::cerr << "[RESPONDER] Failed to create options: " << natsStatus_GetText(status) << std::endl;
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
            std::cerr << "[RESPONDER] Failed to connect: " << natsStatus_GetText(status) << std::endl;
            natsOptions_Destroy(opts);
            return false;
        }
        
        std::cout << "[RESPONDER] " << GetCurrentTimeString() 
                  << " Connected to NATS at " << serverUrls_ << std::endl;
        natsOptions_Destroy(opts);
        return true;
    }
    
    void StartServices() {
        std::cout << "[RESPONDER] " << GetCurrentTimeString() << " Starting responder services..." << std::endl;
        
        // Start RPC responder
        StartRpcResponder();
        
        // Start event consumer
        StartEventConsumer();
        
        std::cout << "[RESPONDER] " << GetCurrentTimeString() << " All services started" << std::endl;
    }
    
    void Run() {
        std::cout << "[RESPONDER] " << GetCurrentTimeString() 
                  << " Responder running... Press Ctrl+C to stop" << std::endl;
        
        // Start stats thread
        std::thread statsThread([this]() {
            PrintPeriodicStats();
        });
        
        // Wait for shutdown signal
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        
        statsThread.join();
        PrintFinalStats();
        std::cout << "[RESPONDER] " << GetCurrentTimeString() << " Responder stopped" << std::endl;
    }
    
private:
    static void LameDuckCallback(natsConnection* nc, void* closure) {
        NatsResponder* self = static_cast<NatsResponder*>(closure);
        self->lameDuckDetected_ = true;
        std::cout << "[RESPONDER] " << self->GetCurrentTimeString() 
                  << " ⚠️  LAME DUCK MODE DETECTED!" << std::endl;
        
        // In a real implementation, we would activate hot-swap client here
        std::cout << "[RESPONDER] " << self->GetCurrentTimeString() 
                  << " 🔄 Would activate hot-swap client here" << std::endl;
    }
    
    static void ReconnectedCallback(natsConnection* nc, void* closure) {
        NatsResponder* self = static_cast<NatsResponder*>(closure);
        std::cout << "[RESPONDER] " << self->GetCurrentTimeString() 
                  << " ✅ Reconnected to NATS server" << std::endl;
    }
    
    static void DisconnectedCallback(natsConnection* nc, void* closure) {
        NatsResponder* self = static_cast<NatsResponder*>(closure);
        std::cout << "[RESPONDER] " << self->GetCurrentTimeString() 
                  << " 🔌 Disconnected from NATS server" << std::endl;
    }
    
    static void RpcMessageCallback(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure) {
        NatsResponder* self = static_cast<NatsResponder*>(closure);
        self->totalRpcRequests_++;
        
        try {
            // Get request data
            const char* data = natsMsg_GetData(msg);
            int len = natsMsg_GetDataLength(msg);
            const char* reply = natsMsg_GetReply(msg);
            
            if (reply == nullptr) {
                std::cout << "[RESPONDER] " << self->GetCurrentTimeString() 
                          << " ❌ RPC request has no reply subject" << std::endl;
                self->failedRpcResponses_++;
                natsMsg_Destroy(msg);
                return;
            }
            
            // Create response
            std::string requestStr(data, len);
            std::string responseData = "{\"response\":\"Echo from responder\",\"original\":\"" + 
                                     requestStr + "\",\"timestamp\":\"" + 
                                     self->GetCurrentTimeString() + "\",\"responder\":\"nats-responder\"}";
            
            // Send response
            natsStatus status = natsConnection_PublishString(nc, reply, responseData.c_str());
            
            if (status == NATS_OK) {
                self->successfulRpcResponses_++;
                
                // Log periodically
                static int rpcCount = 0;
                rpcCount++;
                if (rpcCount % 100 == 0) {
                    std::cout << "[RESPONDER] " << self->GetCurrentTimeString() 
                              << " ✅ Processed " << rpcCount << " RPC requests" << std::endl;
                }
            } else {
                self->failedRpcResponses_++;
                std::cout << "[RESPONDER] " << self->GetCurrentTimeString() 
                          << " ❌ Failed to send RPC response: " << natsStatus_GetText(status) << std::endl;
            }
            
        } catch (const std::exception& e) {
            self->failedRpcResponses_++;
            std::cout << "[RESPONDER] " << self->GetCurrentTimeString() 
                      << " ❌ Error processing RPC request: " << e.what() << std::endl;
        }
        
        natsMsg_Destroy(msg);
    }
    
    static void EventMessageCallback(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure) {
        NatsResponder* self = static_cast<NatsResponder*>(closure);
        self->totalEvents_++;
        
        try {
            // Process the event message
            const char* data = natsMsg_GetData(msg);
            int len = natsMsg_GetDataLength(msg);
            
            self->processedEvents_++;
            
            // Log periodically
            static int eventCount = 0;
            eventCount++;
            if (eventCount % 50 == 0) {
                std::cout << "[RESPONDER] " << self->GetCurrentTimeString() 
                          << " 📨 Processed " << eventCount << " events" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cout << "[RESPONDER] " << self->GetCurrentTimeString() 
                      << " ❌ Error processing event: " << e.what() << std::endl;
        }
        
        natsMsg_Destroy(msg);
    }
    
    void StartRpcResponder() {
        natsStatus status = natsConnection_Subscribe(&rpcSub_, conn_, "test.rpc", 
                                                   RpcMessageCallback, this);
        if (status != NATS_OK) {
            std::cerr << "[RESPONDER] Failed to subscribe to RPC: " << natsStatus_GetText(status) << std::endl;
        } else {
            std::cout << "[RESPONDER] " << GetCurrentTimeString() 
                      << " 🎯 RPC responder started on test.rpc" << std::endl;
        }
    }
    
    void StartEventConsumer() {
        natsStatus status = natsConnection_Subscribe(&eventSub_, conn_, "test.events", 
                                                   EventMessageCallback, this);
        if (status != NATS_OK) {
            std::cerr << "[RESPONDER] Failed to subscribe to events: " << natsStatus_GetText(status) << std::endl;
        } else {
            std::cout << "[RESPONDER] " << GetCurrentTimeString() 
                      << " 📥 Event consumer started on test.events" << std::endl;
        }
    }
    
    void PrintPeriodicStats() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(15));
            
            std::cout << "\n[RESPONDER] " << GetCurrentTimeString() << " 📊 Periodic stats:" << std::endl;
            std::cout << "[RESPONDER]   RPC: " << successfulRpcResponses_ << "/" << totalRpcRequests_ 
                      << " (" << GetSuccessRate(successfulRpcResponses_, totalRpcRequests_) << "%)" << std::endl;
            std::cout << "[RESPONDER]   Events: " << processedEvents_ << "/" << totalEvents_ 
                      << " (" << GetSuccessRate(processedEvents_, totalEvents_) << "%)" << std::endl;
            std::cout << "[RESPONDER]   Lame Duck: " << (lameDuckDetected_ ? "YES" : "NO") << std::endl;
        }
    }
    
    void PrintFinalStats() {
        std::cout << "\n[RESPONDER] 🎯 FINAL RESPONDER RESULTS:" << std::endl;
        std::cout << "[RESPONDER] ===============================" << std::endl;
        std::cout << "[RESPONDER] RPC Operations:" << std::endl;
        std::cout << "[RESPONDER]   Total Requests: " << totalRpcRequests_ << std::endl;
        std::cout << "[RESPONDER]   Successful Responses: " << successfulRpcResponses_ << std::endl;
        std::cout << "[RESPONDER]   Failed Responses: " << failedRpcResponses_ << std::endl;
        std::cout << "[RESPONDER]   Success Rate: " << GetSuccessRate(successfulRpcResponses_, totalRpcRequests_) << "%" << std::endl;
        
        std::cout << "[RESPONDER] Event Operations:" << std::endl;
        std::cout << "[RESPONDER]   Total Events: " << totalEvents_ << std::endl;
        std::cout << "[RESPONDER]   Processed Events: " << processedEvents_ << std::endl;
        std::cout << "[RESPONDER]   Success Rate: " << GetSuccessRate(processedEvents_, totalEvents_) << "%" << std::endl;
        
        std::cout << "[RESPONDER] Lame Duck Detection: " << (lameDuckDetected_ ? "✅ YES" : "❌ NO") << std::endl;
        std::cout << "[RESPONDER] ===============================" << std::endl;
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
        
        if (rpcSub_) {
            natsSubscription_Destroy(rpcSub_);
            rpcSub_ = nullptr;
        }
        
        if (eventSub_) {
            natsSubscription_Destroy(eventSub_);
            eventSub_ = nullptr;
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
    std::cout << "[RESPONDER] 🚀 Starting NATS Responder Service" << std::endl;
    
    // Parse arguments
    std::string natsUrls = "nats://localhost:4222";
    
    if (argc > 1) {
        natsUrls = argv[1];
    }
    
    std::cout << "[RESPONDER] ⚙️  Configuration:" << std::endl;
    std::cout << "[RESPONDER]   NATS URLs: " << natsUrls << std::endl;
    
    // Initialize NATS library
    natsStatus status = nats_Open(-1);
    if (status != NATS_OK) {
        std::cerr << "[RESPONDER] Failed to initialize NATS: " << natsStatus_GetText(status) << std::endl;
        return 1;
    }
    
    // Create and run responder
    NatsResponder responder(natsUrls);
    
    if (!responder.Connect()) {
        std::cerr << "[RESPONDER] ❌ Failed to connect to NATS" << std::endl;
        return 1;
    }
    
    responder.StartServices();
    responder.Run();
    
    std::cout << "[RESPONDER] ✅ Responder finished" << std::endl;
    return 0;
}