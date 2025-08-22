# Pitaya Lame Duck Mode Integration Test

This integration test validates the zero-downtime lame duck mode implementation in libpitaya-cluster.

## Overview

The test simulates a real-world scenario where:
1. A **C++ client** continuously makes Request-Reply RPC calls and publishes events
2. A **C++ server** handles RPC requests and consumes events  
3. A **NATS cluster** undergoes rolling updates (simulating production maintenance)
4. The system validates that **zero downtime** is achieved during the rollout

## Architecture

```
┌─────────────┐    RPC/Events     ┌─────────────────┐
│   Client    │◄─────────────────►│   NATS Cluster  │
│             │                   │   (3 nodes)     │
└─────────────┘                   └─────────────────┘
                                           │
                                           │ RPC/Events
                                           ▼
                                  ┌─────────────────┐
                                  │     Server      │
                                  │                 │
                                  └─────────────────┘
                                           │
                                           │ Service Discovery
                                           ▼
                                  ┌─────────────────┐
                                  │      etcd       │
                                  └─────────────────┘
```

## Components

### Client (`src/client.cpp`)
- Makes continuous RPC requests to `connector.testhandler.echo` route
- Publishes events to `events.client.test` topic
- Monitors lame duck mode and hot-swap activation
- Tracks success/failure rates

### Server (`src/server.cpp`)  
- Registers RPC handler for `testhandler.echo` route
- Subscribes to and processes `events.client.test` events
- Monitors lame duck mode and hot-swap activation
- Tracks request/event processing stats

### Infrastructure
- **NATS Cluster**: 3-node cluster with lame duck configuration
- **etcd**: Service discovery and coordination
- **Docker Compose**: Orchestrates the infrastructure

## Key Features Tested

### Lame Duck Mode Implementation
- **Hot-swap Client**: Instant failover to healthy connection
- **Request Buffering**: Queues Request-Reply operations during transitions
- **Zero-downtime**: No failed operations during NATS rollout

### Test Scenarios
- **Continuous Load**: RPC requests every 50-200ms, events every 100-500ms
- **Rolling Updates**: Sequential restart of NATS nodes with lame duck graceful shutdown
- **Failure Detection**: Monitors for errors, timeouts, and connection issues
- **Success Validation**: Requires ≥95% success rate for RPC and events

## Usage

### Prerequisites
```bash
# Install dependencies
brew install docker docker-compose cmake

# Build the main library first
cd ../cpp-lib
make build-mac-debug
```

### Quick Start
```bash
# Build integration test
make build

# Start infrastructure and run test
make test-lame-duck
```

### Manual Steps
```bash
# 1. Start NATS cluster and etcd
make setup-nats

# 2. Build client and server
make build

# 3. Run test manually
./scripts/test-lame-duck.sh

# 4. Clean up
make teardown-nats
```

### Custom Configuration
```bash
# Run with custom parameters
TEST_DURATION=180 ROLLOUT_DELAY=60 ./scripts/test-lame-duck.sh

# Or run components separately
./build/server "my-server" "nats://localhost:4222" "http://localhost:2379" &
./build/client 120 "nats://localhost:4222"
```

## Test Validation

The test is considered **PASSED** when:
- ✅ Lame duck mode is detected during NATS rollout
- ✅ RPC success rate ≥ 95%
- ✅ Event success rate ≥ 95%  
- ✅ Hot-swap client activation detected
- ✅ Zero failed operations during rollout period

## Monitoring

### Real-time Logs
```bash
# Client logs
tail -f client.log

# Server logs  
tail -f server.log

# NATS cluster status
curl http://localhost:8222/healthz
curl http://localhost:8223/healthz
curl http://localhost:8224/healthz
```

### NATS Monitoring Dashboard
Access at http://localhost:7777 when cluster is running.

### Container Status
```bash
docker ps
docker logs nats-1
docker logs nats-2
docker logs nats-3
```

## Troubleshooting

### Common Issues

**Build Failures**
```bash
# Ensure main library is built
cd ../cpp-lib && make build-mac-debug

# Check for missing dependencies
make check-deps
```

**Connection Issues**
```bash
# Verify NATS cluster health
curl http://localhost:8222/healthz

# Check etcd connectivity
curl http://localhost:2379/health

# View container logs
docker logs nats-1
```

**Test Failures**
```bash
# Check log files for errors
grep -i error client.log server.log

# Verify lame duck detection
grep "LAME DUCK MODE DETECTED" client.log server.log

# Check success rates
grep "Success Rate" client.log server.log
```

### Performance Tuning

**Buffer Sizes**
- Reconnect buffer: 8MB (configured in `common.h`)
- Max pending requests: Calculated from buffer size
- Request timeout: 5s (configurable)

**NATS Settings**
- Lame duck duration: 10s
- Grace period: 2s  
- Max connections: 1000
- Write deadline: 2s

## Expected Output

### Successful Test Run
```
[12:34:56] Starting Pitaya Lame Duck Mode Integration Test
[12:34:56] ✅ NATS cluster is ready
[12:34:56] ✅ etcd is ready  
[12:34:56] ✅ Server is running
[12:34:56] ✅ Client is running
[12:35:26] ⚠️  Beginning NATS cluster rollout (simulating lame duck mode)
[12:35:28] ⚠️  Lame duck mode detected in logs at 32s
[12:35:30] 🔄 Hot-swap client activation detected at 34s
[12:36:56] ✅ NATS cluster rollout completed
[12:37:56] ✅ Integration test PASSED! 🎉
[12:37:56]   - Lame duck mode detected: true
[12:37:56]   - Zero downtime achieved: true
```

This integration test provides comprehensive validation that the lame duck mode implementation delivers true zero-downtime operation during production maintenance scenarios.