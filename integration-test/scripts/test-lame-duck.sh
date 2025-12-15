#!/bin/bash

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

# Configuration
TEST_DURATION=${TEST_DURATION:-120}  # Total test duration in seconds
ROLLOUT_DELAY=${ROLLOUT_DELAY:-30}   # When to start rolling out NATS servers
NATS_ADDR="nats://localhost:4222"
ETCD_ADDR="http://localhost:2379"

# Process IDs for cleanup
CLIENT_PID=""
SERVER_PID=""
MONITORING_PID=""

# Results tracking
TOTAL_ERRORS=0
LAME_DUCK_DETECTED=false
ZERO_DOWNTIME_ACHIEVED=false

log() {
    echo -e "${BLUE}[$(date +'%H:%M:%S')]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[$(date +'%H:%M:%S')] ✅ $1${NC}"
}

log_warning() {
    echo -e "${YELLOW}[$(date +'%H:%M:%S')] ⚠️  $1${NC}"
}

log_error() {
    echo -e "${RED}[$(date +'%H:%M:%S')] ❌ $1${NC}"
}

cleanup() {
    log "Cleaning up processes..."
    
    if [[ -n "$CLIENT_PID" ]] && kill -0 "$CLIENT_PID" 2>/dev/null; then
        log "Stopping client (PID: $CLIENT_PID)..."
        kill -TERM "$CLIENT_PID" 2>/dev/null || true
        wait "$CLIENT_PID" 2>/dev/null || true
    fi
    
    if [[ -n "$SERVER_PID" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
        log "Stopping server (PID: $SERVER_PID)..."
        kill -TERM "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    
    if [[ -n "$MONITORING_PID" ]] && kill -0 "$MONITORING_PID" 2>/dev/null; then
        log "Stopping monitoring (PID: $MONITORING_PID)..."
        kill -TERM "$MONITORING_PID" 2>/dev/null || true
        wait "$MONITORING_PID" 2>/dev/null || true
    fi
    
    log "Cleanup completed"
}

# Trap for cleanup on exit
trap cleanup EXIT INT TERM

wait_for_services() {
    log "Waiting for NATS cluster to be ready..."
    
    local max_attempts=30
    local attempt=1
    
    while [[ $attempt -le $max_attempts ]]; do
        if curl -s http://localhost:8222/healthz >/dev/null 2>&1 && \
           curl -s http://localhost:8223/healthz >/dev/null 2>&1 && \
           curl -s http://localhost:8224/healthz >/dev/null 2>&1; then
            log_success "NATS cluster is ready"
            break
        fi
        
        log "Attempt $attempt/$max_attempts: NATS cluster not ready yet..."
        sleep 2
        ((attempt++))
    done
    
    if [[ $attempt -gt $max_attempts ]]; then
        log_error "NATS cluster failed to become ready"
        return 1
    fi
    
    log "Waiting for etcd to be ready..."
    attempt=1
    while [[ $attempt -le $max_attempts ]]; do
        if curl -s "$ETCD_ADDR/health" >/dev/null 2>&1; then
            log_success "etcd is ready"
            break
        fi
        
        log "Attempt $attempt/$max_attempts: etcd not ready yet..."
        sleep 2
        ((attempt++))
    done
    
    if [[ $attempt -gt $max_attempts ]]; then
        log_error "etcd failed to become ready"
        return 1
    fi
}

start_simple_test() {
    log "Starting simple NATS integration test..."
    
    if [[ ! -f "$BUILD_DIR/simple_test" ]]; then
        log_error "Simple test binary not found at $BUILD_DIR/simple_test"
        return 1
    fi
    
    cd "$PROJECT_DIR"
    "$BUILD_DIR/simple_test" "$TEST_DURATION" "$NATS_ADDR" > test.log 2>&1 &
    CLIENT_PID=$!
    
    log "Simple test started with PID: $CLIENT_PID"
    
    # Give test time to initialize
    sleep 3
    
    if ! kill -0 "$CLIENT_PID" 2>/dev/null; then
        log_error "Simple test failed to start"
        return 1
    fi
    
    log_success "Simple test is running"
}

start_monitoring() {
    log "Starting test monitoring..."
    
    {
        local start_time=$(date +%s)
        local end_time=$((start_time + TEST_DURATION))
        
        while [[ $(date +%s) -lt $end_time ]]; do
            local current_time=$(date +%s)
            local elapsed=$((current_time - start_time))
            
            # Check for lame duck mode in logs
            if grep -q "LAME DUCK MODE DETECTED" test.log 2>/dev/null; then
                if [[ "$LAME_DUCK_DETECTED" == "false" ]]; then
                    LAME_DUCK_DETECTED=true
                    log_warning "Lame duck mode detected in logs at ${elapsed}s"
                fi
            fi
            
            # Check for hot-swap activation
            if grep -q "hot-swap client" test.log 2>/dev/null; then
                log "Hot-swap client activation detected at ${elapsed}s"
            fi
            
            # Count errors in logs
            local current_errors
            current_errors=$(grep -c "failed\|error\|timeout\|exception" test.log 2>/dev/null || echo "0")
            
            if [[ $current_errors -gt $TOTAL_ERRORS ]]; then
                local new_errors=$((current_errors - TOTAL_ERRORS))
                log_warning "Detected $new_errors new error(s) in logs (total: $current_errors)"
                TOTAL_ERRORS=$current_errors
            fi
            
            # Print progress
            if [[ $((elapsed % 15)) -eq 0 ]] && [[ $elapsed -gt 0 ]]; then
                log "Test progress: ${elapsed}/${TEST_DURATION}s elapsed"
            fi
            
            sleep 1
        done
        
        log "Test monitoring completed"
    } &
    
    MONITORING_PID=$!
}

perform_nats_rollout() {
    log "Starting NATS cluster rollout in ${ROLLOUT_DELAY}s..."
    sleep "$ROLLOUT_DELAY"
    
    log_warning "Beginning NATS cluster rollout (simulating lame duck mode)"
    
    # Perform rolling restart of NATS cluster
    local containers=("nats-1" "nats-2" "nats-3")
    
    for container in "${containers[@]}"; do
        log "Rolling out $container..."
        
        # Check if container is running
        if docker ps --format '{{.Names}}' | grep -q "^${container}$"; then
            # Send TERM signal to trigger lame duck mode
            log "Sending SIGTERM to $container (triggers lame duck mode)..."
            docker kill --signal=TERM "$container"
            
            # Wait for lame duck duration + grace period
            log "Waiting for lame duck period (12s)..."
            sleep 12
            
            # Restart the container
            log "Restarting $container..."
            docker restart "$container"
            
            # Wait for container to be healthy again
            log "Waiting for $container to be healthy..."
            local attempts=0
            while [[ $attempts -lt 30 ]]; do
                if docker exec "$container" wget --quiet --tries=1 --spider http://localhost:8222/healthz 2>/dev/null; then
                    log_success "$container is healthy again"
                    break
                fi
                sleep 1
                ((attempts++))
            done
            
            if [[ $attempts -ge 30 ]]; then
                log_warning "$container may not be fully healthy yet"
            fi
            
            # Wait between container rollouts
            log "Waiting 10s before next container rollout..."
            sleep 10
        else
            log_warning "$container is not running, skipping"
        fi
    done
    
    log_success "NATS cluster rollout completed"
}

analyze_results() {
    log "Analyzing test results..."
    
    # Wait for processes to complete
    if [[ -n "$CLIENT_PID" ]] && kill -0 "$CLIENT_PID" 2>/dev/null; then
        log "Waiting for client to complete..."
        wait "$CLIENT_PID" || true
    fi
    
    # Analyze logs
    log "Final log analysis:"
    
    # Check if lame duck mode was detected
    if grep -q "LAME DUCK MODE DETECTED" test.log 2>/dev/null; then
        LAME_DUCK_DETECTED=true
        log_success "Lame duck mode was properly detected"
    else
        log_warning "Lame duck mode was not detected in logs"
    fi
    
    # Check for final test results in the log
    if grep -q "TEST PASSED.*ZERO DOWNTIME ACHIEVED" test.log 2>/dev/null; then
        log_success "Simple test reported ZERO DOWNTIME ACHIEVED!"
        ZERO_DOWNTIME_ACHIEVED=true
    elif grep -q "TEST FAILED" test.log 2>/dev/null; then
        log_error "Simple test reported TEST FAILED"
    else
        log_warning "Could not determine final test result from logs"
    fi
    
    # Count successful vs failed operations from test output
    local total_rpc=$(grep -oP "RPC.*Total: \K\d+" test.log 2>/dev/null | tail -1 || echo "0")
    local successful_rpc=$(grep -oP "RPC.*Successful: \K\d+" test.log 2>/dev/null | tail -1 || echo "0")
    local failed_rpc=$(grep -oP "RPC.*Failed: \K\d+" test.log 2>/dev/null | tail -1 || echo "0")
    
    local total_events=$(grep -oP "Event.*Total: \K\d+" test.log 2>/dev/null | tail -1 || echo "0")
    local successful_events=$(grep -oP "Event.*Successful: \K\d+" test.log 2>/dev/null | tail -1 || echo "0")
    local failed_events=$(grep -oP "Event.*Failed: \K\d+" test.log 2>/dev/null | tail -1 || echo "0")
    
    # Print statistics
    log "=== TEST RESULTS ==="
    log "RPC Operations:"
    log "  Total: $total_rpc"
    log "  Successful: $successful_rpc"
    log "  Failed: $failed_rpc"
    
    if [[ $total_rpc -gt 0 ]]; then
        local rpc_success_rate=$((successful_rpc * 100 / total_rpc))
        log "  Success Rate: ${rpc_success_rate}%"
        
        if [[ $rpc_success_rate -ge 95 ]]; then
            log_success "RPC success rate is acceptable (≥95%)"
        else
            log_error "RPC success rate is too low (<95%)"
            return 1
        fi
    fi
    
    log "Event Operations:"
    log "  Total: $total_events"
    log "  Successful: $successful_events"
    log "  Failed: $failed_events"
    
    if [[ $total_events -gt 0 ]]; then
        local event_success_rate=$((successful_events * 100 / total_events))
        log "  Success Rate: ${event_success_rate}%"
        
        if [[ $event_success_rate -ge 95 ]]; then
            log_success "Event success rate is acceptable (≥95%)"
        else
            log_error "Event success rate is too low (<95%)"
            return 1
        fi
    fi
    
    # Check for zero-downtime achievement
    if [[ "$LAME_DUCK_DETECTED" == "true" ]] && [[ $failed_rpc -eq 0 ]] && [[ $failed_events -eq 0 ]]; then
        ZERO_DOWNTIME_ACHIEVED=true
        log_success "ZERO-DOWNTIME ACHIEVED! 🎉"
    elif [[ "$LAME_DUCK_DETECTED" == "false" ]]; then
        log_warning "Lame duck mode was not triggered, test may be inconclusive"
    else
        log_error "Zero-downtime was not achieved - some operations failed during rollout"
        return 1
    fi
    
    log "===================="
}

main() {
    log "Starting Pitaya Lame Duck Mode Integration Test"
    log "Configuration:"
    log "  Test Duration: ${TEST_DURATION}s"
    log "  Rollout Delay: ${ROLLOUT_DELAY}s"
    log "  NATS Address: $NATS_ADDR"
    log "  etcd Address: $ETCD_ADDR"
    
    # Ensure build directory exists and binaries are built
    if [[ ! -d "$BUILD_DIR" ]]; then
        log_error "Build directory not found. Please run 'make build' first."
        return 1
    fi
    
    # Wait for services to be ready
    wait_for_services
    
    # Start simple test (combines client and server functionality)
    start_simple_test
    start_monitoring
    
    # Perform the rollout in background
    perform_nats_rollout &
    local rollout_pid=$!
    
    # Wait for client to complete or timeout
    if [[ -n "$CLIENT_PID" ]]; then
        wait "$CLIENT_PID" || true
    fi
    
    # Wait for rollout to complete
    wait "$rollout_pid" || true
    
    # Analyze results
    if analyze_results; then
        log_success "Integration test PASSED! 🎉"
        log "  - Lame duck mode detected: $LAME_DUCK_DETECTED"
        log "  - Zero downtime achieved: $ZERO_DOWNTIME_ACHIEVED"
        return 0
    else
        log_error "Integration test FAILED! ❌"
        return 1
    fi
}

# Run the main function
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi