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

# Configuration
MIN_SUCCESS_RATE=${MIN_SUCCESS_RATE:-95}
CLIENT_LOG="${PROJECT_DIR}/client.log"
SERVER_LOG="${PROJECT_DIR}/server.log"

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

validate_logs_exist() {
    if [[ ! -f "$CLIENT_LOG" ]]; then
        log_error "Client log file not found: $CLIENT_LOG"
        return 1
    fi
    
    if [[ ! -f "$SERVER_LOG" ]]; then
        log_error "Server log file not found: $SERVER_LOG"
        return 1
    fi
    
    log_success "Log files found"
    return 0
}

extract_client_stats() {
    log "Extracting client statistics..."
    
    # Extract final statistics from client log
    local stats_section
    if stats_section=$(grep -A 20 "=== Integration Test Statistics ===" "$CLIENT_LOG" | tail -20); then
        
        # Parse RPC stats
        local total_rpc=$(echo "$stats_section" | grep -oP "Total: \K\d+" | head -1 || echo "0")
        local successful_rpc=$(echo "$stats_section" | grep -oP "Successful: \K\d+" | head -1 || echo "0")
        local failed_rpc=$(echo "$stats_section" | grep -oP "Failed: \K\d+" | head -1 || echo "0")
        
        # Parse Event stats  
        local total_events=$(echo "$stats_section" | grep -oP "Total: \K\d+" | tail -1 || echo "0")
        local successful_events=$(echo "$stats_section" | grep -oP "Successful: \K\d+" | tail -1 || echo "0")
        local failed_events=$(echo "$stats_section" | grep -oP "Failed: \K\d+" | tail -1 || echo "0")
        
        # Parse lame duck detection
        local lame_duck_detected=$(echo "$stats_section" | grep -oP "Detected: \K(Yes|No)" || echo "No")
        local hotswap_activated=$(echo "$stats_section" | grep -oP "Hot-swap Activated: \K(Yes|No)" || echo "No")
        
        # Export for use by other functions
        export CLIENT_TOTAL_RPC="$total_rpc"
        export CLIENT_SUCCESSFUL_RPC="$successful_rpc"
        export CLIENT_FAILED_RPC="$failed_rpc"
        export CLIENT_TOTAL_EVENTS="$total_events"
        export CLIENT_SUCCESSFUL_EVENTS="$successful_events"
        export CLIENT_FAILED_EVENTS="$failed_events"
        export CLIENT_LAME_DUCK_DETECTED="$lame_duck_detected"
        export CLIENT_HOTSWAP_ACTIVATED="$hotswap_activated"
        
        log "Client stats extracted:"
        log "  RPC: $total_rpc total, $successful_rpc successful, $failed_rpc failed"
        log "  Events: $total_events total, $successful_events successful, $failed_events failed"
        log "  Lame Duck: $lame_duck_detected, Hot-swap: $hotswap_activated"
        
        return 0
    else
        log_warning "Could not find client statistics section"
        return 1
    fi
}

extract_server_stats() {
    log "Extracting server statistics..."
    
    # Extract final statistics from server log
    local stats_section
    if stats_section=$(grep -A 20 "=== Integration Test Statistics ===" "$SERVER_LOG" | tail -20); then
        
        # Parse RPC stats
        local total_rpc=$(echo "$stats_section" | grep -oP "Total: \K\d+" | head -1 || echo "0")
        local successful_rpc=$(echo "$stats_section" | grep -oP "Successful: \K\d+" | head -1 || echo "0")
        local failed_rpc=$(echo "$stats_section" | grep -oP "Failed: \K\d+" | head -1 || echo "0")
        
        # Parse Event stats  
        local total_events=$(echo "$stats_section" | grep -oP "Total: \K\d+" | tail -1 || echo "0")
        local successful_events=$(echo "$stats_section" | grep -oP "Successful: \K\d+" | tail -1 || echo "0")
        local failed_events=$(echo "$stats_section" | grep -oP "Failed: \K\d+" | tail -1 || echo "0")
        
        # Parse lame duck detection
        local lame_duck_detected=$(echo "$stats_section" | grep -oP "Detected: \K(Yes|No)" || echo "No")
        local hotswap_activated=$(echo "$stats_section" | grep -oP "Hot-swap Activated: \K(Yes|No)" || echo "No")
        
        # Export for use by other functions
        export SERVER_TOTAL_RPC="$total_rpc"
        export SERVER_SUCCESSFUL_RPC="$successful_rpc"
        export SERVER_FAILED_RPC="$failed_rpc"
        export SERVER_TOTAL_EVENTS="$total_events"
        export SERVER_SUCCESSFUL_EVENTS="$successful_events"
        export SERVER_FAILED_EVENTS="$failed_events"
        export SERVER_LAME_DUCK_DETECTED="$lame_duck_detected"
        export SERVER_HOTSWAP_ACTIVATED="$hotswap_activated"
        
        log "Server stats extracted:"
        log "  RPC: $total_rpc total, $successful_rpc successful, $failed_rpc failed"
        log "  Events: $total_events total, $successful_events successful, $failed_events failed"
        log "  Lame Duck: $lame_duck_detected, Hot-swap: $hotswap_activated"
        
        return 0
    else
        log_warning "Could not find server statistics section"
        return 1
    fi
}

validate_rpc_operations() {
    log "Validating RPC operations..."
    
    local client_total=${CLIENT_TOTAL_RPC:-0}
    local client_successful=${CLIENT_SUCCESSFUL_RPC:-0}
    local server_total=${SERVER_TOTAL_RPC:-0}
    local server_successful=${SERVER_SUCCESSFUL_RPC:-0}
    
    # Validate minimum activity
    if [[ $client_total -lt 50 ]]; then
        log_error "Insufficient RPC activity from client: $client_total (minimum: 50)"
        return 1
    fi
    
    if [[ $server_total -lt 50 ]]; then
        log_error "Insufficient RPC activity on server: $server_total (minimum: 50)"
        return 1
    fi
    
    # Calculate success rates
    local client_success_rate=0
    local server_success_rate=0
    
    if [[ $client_total -gt 0 ]]; then
        client_success_rate=$((client_successful * 100 / client_total))
    fi
    
    if [[ $server_total -gt 0 ]]; then
        server_success_rate=$((server_successful * 100 / server_total))
    fi
    
    log "RPC Success Rates:"
    log "  Client: ${client_success_rate}% (${client_successful}/${client_total})"
    log "  Server: ${server_success_rate}% (${server_successful}/${server_total})"
    
    # Validate success rates
    if [[ $client_success_rate -lt $MIN_SUCCESS_RATE ]]; then
        log_error "Client RPC success rate too low: ${client_success_rate}% (minimum: ${MIN_SUCCESS_RATE}%)"
        return 1
    fi
    
    if [[ $server_success_rate -lt $MIN_SUCCESS_RATE ]]; then
        log_error "Server RPC success rate too low: ${server_success_rate}% (minimum: ${MIN_SUCCESS_RATE}%)"
        return 1
    fi
    
    log_success "RPC operations validation passed"
    return 0
}

validate_event_operations() {
    log "Validating event operations..."
    
    local client_total=${CLIENT_TOTAL_EVENTS:-0}
    local client_successful=${CLIENT_SUCCESSFUL_EVENTS:-0}
    local server_total=${SERVER_TOTAL_EVENTS:-0}
    local server_successful=${SERVER_SUCCESSFUL_EVENTS:-0}
    
    # Validate minimum activity
    if [[ $client_total -lt 20 ]]; then
        log_error "Insufficient event activity from client: $client_total (minimum: 20)"
        return 1
    fi
    
    if [[ $server_total -lt 20 ]]; then
        log_error "Insufficient event activity on server: $server_total (minimum: 20)"
        return 1
    fi
    
    # Calculate success rates
    local client_success_rate=0
    local server_success_rate=0
    
    if [[ $client_total -gt 0 ]]; then
        client_success_rate=$((client_successful * 100 / client_total))
    fi
    
    if [[ $server_total -gt 0 ]]; then
        server_success_rate=$((server_successful * 100 / server_total))
    fi
    
    log "Event Success Rates:"
    log "  Client: ${client_success_rate}% (${client_successful}/${client_total})"
    log "  Server: ${server_success_rate}% (${server_successful}/${server_total})"
    
    # Validate success rates
    if [[ $client_success_rate -lt $MIN_SUCCESS_RATE ]]; then
        log_error "Client event success rate too low: ${client_success_rate}% (minimum: ${MIN_SUCCESS_RATE}%)"
        return 1
    fi
    
    if [[ $server_success_rate -lt $MIN_SUCCESS_RATE ]]; then
        log_error "Server event success rate too low: ${server_success_rate}% (minimum: ${MIN_SUCCESS_RATE}%)"
        return 1
    fi
    
    log_success "Event operations validation passed"
    return 0
}

validate_lame_duck_handling() {
    log "Validating lame duck mode handling..."
    
    local client_detected=${CLIENT_LAME_DUCK_DETECTED:-"No"}
    local server_detected=${SERVER_LAME_DUCK_DETECTED:-"No"}
    local client_hotswap=${CLIENT_HOTSWAP_ACTIVATED:-"No"}
    local server_hotswap=${SERVER_HOTSWAP_ACTIVATED:-"No"}
    
    # Check if lame duck mode was detected
    if [[ "$client_detected" == "Yes" ]] || [[ "$server_detected" == "Yes" ]]; then
        log_success "Lame duck mode was detected"
        
        # Check if hot-swap was activated
        if [[ "$client_hotswap" == "Yes" ]] || [[ "$server_hotswap" == "Yes" ]]; then
            log_success "Hot-swap mechanism was activated"
        else
            log_warning "Hot-swap mechanism was not detected in logs"
        fi
        
        return 0
    else
        log_warning "Lame duck mode was not detected in either client or server logs"
        log_warning "This may indicate the test did not properly trigger lame duck conditions"
        return 1
    fi
}

validate_zero_downtime() {
    log "Validating zero-downtime achievement..."
    
    local client_failed_rpc=${CLIENT_FAILED_RPC:-0}
    local client_failed_events=${CLIENT_FAILED_EVENTS:-0}
    local server_failed_rpc=${SERVER_FAILED_RPC:-0}
    local server_failed_events=${SERVER_FAILED_EVENTS:-0}
    
    local total_failures=$((client_failed_rpc + client_failed_events + server_failed_rpc + server_failed_events))
    
    if [[ $total_failures -eq 0 ]]; then
        log_success "ZERO-DOWNTIME ACHIEVED! No failed operations detected 🎉"
        return 0
    else
        log_error "Zero-downtime NOT achieved. Total failures: $total_failures"
        log "  Client RPC failures: $client_failed_rpc"
        log "  Client event failures: $client_failed_events"
        log "  Server RPC failures: $server_failed_rpc"
        log "  Server event failures: $server_failed_events"
        return 1
    fi
}

check_error_patterns() {
    log "Checking for error patterns in logs..."
    
    local critical_errors=0
    
    # Check for critical error patterns
    local critical_patterns=(
        "NATS_ILLEGAL_STATE"
        "Connection refused"
        "Connection timeout"
        "segmentation fault"
        "core dumped"
        "Fatal error"
    )
    
    for pattern in "${critical_patterns[@]}"; do
        local count
        count=$(grep -c "$pattern" "$CLIENT_LOG" "$SERVER_LOG" 2>/dev/null || echo "0")
        if [[ $count -gt 0 ]]; then
            log_error "Found $count instances of critical error: '$pattern'"
            critical_errors=$((critical_errors + count))
        fi
    done
    
    if [[ $critical_errors -eq 0 ]]; then
        log_success "No critical error patterns found"
        return 0
    else
        log_error "Found $critical_errors critical errors in logs"
        return 1
    fi
}

generate_report() {
    log "Generating validation report..."
    
    local report_file="${PROJECT_DIR}/test-validation-report.txt"
    
    {
        echo "======================================"
        echo "Pitaya Lame Duck Mode Test Report"
        echo "Generated: $(date)"
        echo "======================================"
        echo ""
        echo "CLIENT STATISTICS:"
        echo "  RPC Operations: ${CLIENT_TOTAL_RPC:-0} total, ${CLIENT_SUCCESSFUL_RPC:-0} successful, ${CLIENT_FAILED_RPC:-0} failed"
        echo "  Event Operations: ${CLIENT_TOTAL_EVENTS:-0} total, ${CLIENT_SUCCESSFUL_EVENTS:-0} successful, ${CLIENT_FAILED_EVENTS:-0} failed"
        echo "  Lame Duck Detected: ${CLIENT_LAME_DUCK_DETECTED:-No}"
        echo "  Hot-swap Activated: ${CLIENT_HOTSWAP_ACTIVATED:-No}"
        echo ""
        echo "SERVER STATISTICS:"
        echo "  RPC Operations: ${SERVER_TOTAL_RPC:-0} total, ${SERVER_SUCCESSFUL_RPC:-0} successful, ${SERVER_FAILED_RPC:-0} failed"
        echo "  Event Operations: ${SERVER_TOTAL_EVENTS:-0} total, ${SERVER_SUCCESSFUL_EVENTS:-0} successful, ${SERVER_FAILED_EVENTS:-0} failed"
        echo "  Lame Duck Detected: ${SERVER_LAME_DUCK_DETECTED:-No}"
        echo "  Hot-swap Activated: ${SERVER_HOTSWAP_ACTIVATED:-No}"
        echo ""
        echo "VALIDATION RESULTS:"
        echo "  Minimum Success Rate: ${MIN_SUCCESS_RATE}%"
        
        # Calculate overall success rates
        local total_rpc=$((${CLIENT_TOTAL_RPC:-0} + ${SERVER_TOTAL_RPC:-0}))
        local successful_rpc=$((${CLIENT_SUCCESSFUL_RPC:-0} + ${SERVER_SUCCESSFUL_RPC:-0}))
        local overall_rpc_rate=0
        
        if [[ $total_rpc -gt 0 ]]; then
            overall_rpc_rate=$((successful_rpc * 100 / total_rpc))
        fi
        
        local total_events=$((${CLIENT_TOTAL_EVENTS:-0} + ${SERVER_TOTAL_EVENTS:-0}))
        local successful_events=$((${CLIENT_SUCCESSFUL_EVENTS:-0} + ${SERVER_SUCCESSFUL_EVENTS:-0}))
        local overall_event_rate=0
        
        if [[ $total_events -gt 0 ]]; then
            overall_event_rate=$((successful_events * 100 / total_events))
        fi
        
        echo "  Overall RPC Success Rate: ${overall_rpc_rate}%"
        echo "  Overall Event Success Rate: ${overall_event_rate}%"
        
        local total_failures=$((${CLIENT_FAILED_RPC:-0} + ${CLIENT_FAILED_EVENTS:-0} + ${SERVER_FAILED_RPC:-0} + ${SERVER_FAILED_EVENTS:-0}))
        
        if [[ $total_failures -eq 0 ]]; then
            echo "  Zero-Downtime Status: ACHIEVED ✅"
        else
            echo "  Zero-Downtime Status: NOT ACHIEVED ❌ ($total_failures failures)"
        fi
        
        echo ""
        echo "======================================"
        
    } > "$report_file"
    
    log_success "Validation report generated: $report_file"
    
    # Print summary to console
    cat "$report_file"
}

main() {
    log "Starting test result validation"
    log "Configuration:"
    log "  Minimum Success Rate: ${MIN_SUCCESS_RATE}%"
    log "  Client Log: $CLIENT_LOG"
    log "  Server Log: $SERVER_LOG"
    
    local validation_errors=0
    
    # Run all validations
    validate_logs_exist || ((validation_errors++))
    extract_client_stats || ((validation_errors++))
    extract_server_stats || ((validation_errors++))
    validate_rpc_operations || ((validation_errors++))
    validate_event_operations || ((validation_errors++))
    validate_lame_duck_handling || ((validation_errors++))
    validate_zero_downtime || ((validation_errors++))
    check_error_patterns || ((validation_errors++))
    
    # Generate report regardless of validation results
    generate_report
    
    # Final result
    if [[ $validation_errors -eq 0 ]]; then
        log_success "All validations PASSED! 🎉"
        log_success "Zero-downtime lame duck mode implementation is working correctly"
        return 0
    else
        log_error "Validation FAILED with $validation_errors error(s) ❌"
        log_error "Review the logs and report for details"
        return 1
    fi
}

# Run the main function if executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi