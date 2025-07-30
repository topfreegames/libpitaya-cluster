# NATS Lame Duck Mode Handling

This document explains how the Pitaya NATS client handles lame duck mode, a graceful shutdown mechanism used by NATS servers.

## Overview

Lame duck mode is a NATS server feature that allows for graceful shutdown without causing a "thundering herd" problem where all clients simultaneously reconnect to other servers. When a NATS server enters lame duck mode:

1. It stops accepting new connections
2. It waits 10 seconds grace period
3. It gradually evicts clients over a configurable duration (default: 2 minutes)

## Implementation in Pitaya

The Pitaya NATS client automatically handles lame duck mode through the following mechanisms:

### Automatic Detection

The client automatically detects when a server enters lame duck mode through the server's INFO protocol message and triggers the `LameDuckModeCb` callback.

### Automatic Response

When lame duck mode is detected, the client automatically:

1. **Sets a thread-safe flag** to prevent new operations (subscribe, request)
2. **Allows publishing** with automatic buffering by NATS.c
3. **Drains existing subscriptions** gracefully using `natsSubscription_Drain()` (1s timeout)
4. **Flushes pending messages** to the current server using `natsConnection_FlushTimeout()` (1s timeout)
5. **Triggers reconnection** to other servers using `natsConnection_Reconnect()`
6. **Resets the flag** when reconnection completes

### Message Buffering During Lame Duck Mode

Unlike other operations, **publishing is allowed during lame duck mode**. Messages are automatically buffered by the NATS.c library using the **reconnection buffer** and will be sent to the new server after reconnection.

#### How NATS.c Buffering Works

1. **Buffer Creation**: When lame duck mode triggers reconnection, NATS.c creates a `pending` buffer
2. **Message Storage**: All `Publish()` calls during lame duck mode are stored in this buffer
3. **Buffer Size**: Controlled by `reconnectBufSize` in `NatsConfig` (default: 8MB)
4. **Automatic Flush**: When reconnection completes, NATS.c automatically flushes all buffered messages

#### Buffer Configuration

```cpp
// Configure buffer size for lame duck mode and reconnection
config.reconnectBufSize = 8388608; // 8MB default
```

### Thread Safety

The implementation uses proper synchronization:

- **Mutex Protection**: `_lameDuckMode` flag is protected by `_lameDuckModeMutex`
- **Thread-Safe Access**: All methods that check or modify the flag use proper locking
- **Callback Safety**: Multiple callbacks can safely access the flag concurrently

### Operation Protection

During lame duck mode, most NATS operations are blocked:

```cpp
// These will return NATS_ILLEGAL_STATE during lame duck mode
natsStatus status = client->Subscribe("subject", callback);
natsStatus status = client->Request(&msg, "subject", data, timeout);

// This is ALLOWED and will be buffered automatically
natsStatus status = client->Publish("subject", data);
```

### Checking Status

You can check the lame duck mode status of the client:

```cpp
// Check if in lame duck mode (thread-safe)
if (client->IsInLameDuckMode()) {
    std::cout << "Client is in lame duck mode" << std::endl;
}
```

## Usage Example

```cpp
#include "pitaya/nats_client.h"

// Create NATS client
auto config = pitaya::NatsConfig();
config.natsAddr = "nats://localhost:4222";
config.reconnectBufSize = 16777216; // 16MB buffer for lame duck mode
auto client = std::make_unique<pitaya::NatsClientImpl>(
    pitaya::NatsApiType::Asynchronous, config, "my_logger");

// Subscribe to a topic
client->Subscribe("my.topic", [](std::shared_ptr<pitaya::NatsMsg> msg) {
    // Handle message
});

// Publish messages (will be buffered during lame duck mode)
std::vector<uint8_t> data = {'h', 'e', 'l', 'l', 'o'};
natsStatus status = client->Publish("my.topic", data);

// Check lame duck mode status
if (client->IsInLameDuckMode()) {
    std::cout << "Client is currently in lame duck mode" << std::endl;
    // Publishing is still allowed and will be buffered
    client->Publish("my.topic", data);
}
```

## Logging

The client provides detailed logging during lame duck mode events:

```
[INFO] === LAME DUCK MODE DETECTED ===
[INFO] Server: nats://localhost:4222
[INFO] Lame duck mode flag set - preventing new operations
[INFO] Draining subscription...
[INFO] Successfully initiated subscription drain
[INFO] Subscription drain completed successfully
[INFO] Flushing pending messages...
[INFO] Successfully flushed pending messages
[INFO] Initiating reconnection...
[INFO] === LAME DUCK MODE HANDLING COMPLETE ===
[WARN] nats reconnected!
[INFO] Lame duck mode flag reset after successful reconnection
```

## Configuration

### Timeouts

The client uses configurable timeouts for lame duck mode operations:

- **Drain Timeout**: 1 second (default) - Time to wait for subscription drain completion
- **Flush Timeout**: 1 second (default) - Time to wait for message flush completion

These timeouts are shared between client destruction and lame duck mode handling.

### Buffer Management

The NATS.c library automatically handles message buffering during lame duck mode and reconnection:

- **Buffer Size**: Configured via `reconnectBufSize` in `NatsConfig`
- **Default Size**: 8MB (8,388,608 bytes)
- **Automatic Flush**: Messages are automatically sent to the new server after reconnection
- **Overflow Protection**: Returns `NATS_INSUFFICIENT_BUFFER` if buffer is full

## Best Practices

1. **Allow publishing during lame duck mode** - Messages will be automatically buffered
2. **Configure appropriate buffer size** - Ensure `reconnectBufSize` can handle your message volume
3. **Check return codes** from NATS operations to handle `NATS_ILLEGAL_STATE`
4. **Use the `IsInLameDuckMode()` method** to check status before attempting operations
5. **Implement retry logic** for operations that fail during lame duck mode
6. **Monitor logs** for lame duck mode events in production environments

## Technical Details

### **NATS.c Integration**
- **Version**: Compatible with NATS.c 3.10.1
- **API**: Uses `natsOptions_SetLameDuckModeCB()` for callback registration
- **Buffering**: Leverages NATS.c's built-in `nc->pending` buffer
- **Reconnection**: Automatic reconnection handled by NATS.c library

### **Thread Safety**
- **Mutex Type**: `std::mutex` with `std::lock_guard` (C++17 compatible)
- **Lock Scope**: Minimal lock time for optimal performance
- **Exception Safety**: RAII ensures proper cleanup
- **Concurrent Access**: Safe from multiple callback threads

### **Error Handling**
- **Status Codes**: Proper `natsStatus` propagation
- **Timeout Handling**: Graceful timeout with detailed logging
- **Buffer Overflow**: Handles `NATS_INSUFFICIENT_BUFFER` scenarios
- **Reconnection**: Automatic recovery after server shutdown

### **Buffer Overflow Handling**
When the NATS.c reconnection buffer becomes full during lame duck mode:

- **Default Buffer Size**: 8MB (`NATS_OPTS_DEFAULT_RECONNECT_BUF_SIZE`)
- **Overflow Behavior**: New messages are rejected with `NATS_INSUFFICIENT_BUFFER`
- **No Eviction**: Old messages are preserved (FIFO order maintained)
- **Error Response**: Returns `NATS_INSUFFICIENT_BUFFER` with warning log
- **Application Responsibility**: C# application code should handle this as non-retryable

```cpp
// Example error handling in Publish method
case NATS_INSUFFICIENT_BUFFER:
    _log->warn("NATS reconnection buffer full during lame duck mode - message dropped. "
              "C# application should handle this as a non-retryable error");
    return status;
```

**Recommended C# Application Handling:**
```csharp
if (status == NatsStatus.InsufficientBuffer)
{
    // Don't retry - buffer is full, message is dropped
    logger.Warn("NATS buffer full during lame duck mode - message dropped");
    return ErrorResponse("Service temporarily unavailable - buffer full");
}
```

### **Error Handling in RPC Client and Server**

The RPC client and server components have been enhanced to properly handle lame duck mode errors:

#### **RPC Client Error Handling**
```cpp
// Enhanced error handling in Call(), SendKickToUser(), SendPushToUser()
switch (status) {
    case NATS_TIMEOUT:
        // Handle timeout errors
        break;
    case NATS_ILLEGAL_STATE:
        // Lame duck mode - service temporarily unavailable
        err->set_msg("service temporarily unavailable - lame duck mode");
        break;
    case NATS_INSUFFICIENT_BUFFER:
        // Buffer overflow during lame duck mode
        err->set_msg("service temporarily unavailable - buffer full");
        break;
    default:
        // Handle other NATS errors
        break;
}
```

#### **RPC Server Error Handling**
```cpp
// Enhanced error handling in response publishing
switch (status) {
    case NATS_ILLEGAL_STATE:
        log->warn("Failed to publish RPC response - lame duck mode active");
        break;
    case NATS_INSUFFICIENT_BUFFER:
        log->warn("Failed to publish RPC response - buffer full during lame duck mode");
        break;
    case NATS_TIMEOUT:
        log->error("Failed to publish RPC response - timeout");
        break;
    default:
        log->error("Failed to publish RPC response: {}", natsStatus_GetText(status));
        break;
}
```

#### **Error Code Mapping**
- **`NATS_ILLEGAL_STATE`**: Lame duck mode active - service temporarily unavailable
- **`NATS_INSUFFICIENT_BUFFER`**: Buffer overflow during lame duck mode
- **`NATS_TIMEOUT`**: Request timeout
- **Other errors**: Generic NATS error with status text

This ensures that C# applications receive meaningful error messages and can implement appropriate retry logic based on the specific error type.

### **Request-Reply Buffer Analysis**

**✅ NATS.c Automatically Buffers Request-Reply Messages**

The Request-Reply pattern is one of the most critical flows in the application. Analysis of NATS.c source code reveals:

#### **Internal Request-Reply Flow**
```cpp
// NATS.c internal flow:
natsConnection_Request() 
  → natsConnection_RequestMsg()
    → natsConn_publish()  // Uses same buffer as regular Publish
      → nc->pending buffer (during reconnection/LDM)
```

#### **Buffer Behavior**
- ✅ **Same buffer** (`nc->pending`) used for both Publish and Request-Reply
- ✅ **Same size limit** (`reconnectBufSize`) applies to both
- ✅ **Same overflow behavior** (`NATS_INSUFFICIENT_BUFFER`) for both
- ✅ **No additional application buffering needed**

#### **Request-Reply During Lame Duck Mode**
```cpp
// During LDM, Request-Reply messages are:
// 1. Buffered in NATS.c's internal buffer
// 2. Sent when reconnection completes
// 3. Subject to same overflow limits as Publish
```

### **Zero-Downtime Lame Duck Mode (Hot-Swap)**

**Problem**: During lame duck mode, there's a window where messages fail while the client drains and reconnects.

**Solution**: Hot-swap to a new NATS client connected to a healthy node.

#### **Hot-Swap Implementation**
```cpp
class NatsClientImpl {
private:
    mutable std::mutex _hotSwapMutex;
    std::shared_ptr<NatsClient> _hotSwapClient;
    bool _hotSwapAvailable;

public:
    void SetHotSwapClient(std::shared_ptr<NatsClient> newClient);
    std::shared_ptr<NatsClient> GetHotSwapClient() const;
    bool IsHotSwapAvailable() const;
};
```

#### **Zero-Downtime Flow**
```cpp
void LameDuckModeCb(natsConnection* nc, void* user) {
    auto client = static_cast<NatsClientImpl*>(user);
    
    // 1. Create new client connected to healthy node
    auto newClient = std::make_shared<NatsClientImpl>(
        NatsApiType::Synchronous, config, "hot_swap_logger");
    
    // 2. Set as hot-swap client
    client->SetHotSwapClient(newClient);
    
    // 3. Continue using current client (will use hot-swap when needed)
    // 4. Old client drains in background
    // 5. Old client destructs when drained
}
```

#### **Request Method with Hot-Swap**
```cpp
natsStatus NatsClientImpl::Request(...) {
    if (IsInLameDuckMode()) {
        // Try hot-swap client first
        auto hotSwapClient = GetHotSwapClient();
        if (hotSwapClient && IsHotSwapAvailable()) {
            return hotSwapClient->Request(msg, topic, data, timeout);
        }
        return NATS_ILLEGAL_STATE;
    }
    // Normal request flow
}
```

#### **Benefits**
- ✅ **Zero message loss** during lame duck mode
- ✅ **Seamless failover** to healthy nodes
- ✅ **No application downtime**
- ✅ **Automatic cleanup** of old connections
- ✅ **Thread-safe** hot-swap operations

#### **Usage in C# Application**
```csharp
// C# application automatically benefits from hot-swap
// No changes needed - zero-downtime is transparent
var response = await cluster.RPC(serverId, route, request);
// During LDM, this automatically uses the hot-swap client
```
