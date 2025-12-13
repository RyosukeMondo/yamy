#!/bin/bash
################################################################################
# YAMY Comprehensive Test Runner
# ================================
#
# CI-ready script to run all test phases:
# - Unit tests (C++ GoogleTest)
# - Integration tests (C++ GoogleTest)
# - E2E tests (Python autonomous framework)
# - Test report generation
#
# Features:
# - Automatic YAMY startup/shutdown
# - Proper error handling and cleanup
# - Exit code reflects test status (0 = all pass, non-zero = failures)
# - No user interaction required (fully autonomous)
#
# Task 3.7 - Key Remapping Consistency Spec
################################################################################

set -e  # Exit on error (but we'll handle cleanup)
set -u  # Error on undefined variables

################################################################################
# Configuration
################################################################################

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
LOG_DIR="${LOG_DIR:-/tmp}"

# Test executables
YAMY_BIN="$BUILD_DIR/bin/yamy"
YAMY_TEST_BIN="$BUILD_DIR/bin/yamy-test"
UNIT_TEST_BIN="$BUILD_DIR/bin/yamy_event_processor_ut"
INTEGRATION_TEST_BIN="$BUILD_DIR/bin/yamy_event_processor_it"

# Python test scripts
AUTOMATED_TEST="$SCRIPT_DIR/automated_keymap_test.py"
REPORT_GENERATOR="$SCRIPT_DIR/generate_test_report.py"

# Log files
# YAMY writes its debug output to /tmp/yamy-debug.log by default
YAMY_LOG="$LOG_DIR/yamy-debug.log"
TEST_RESULTS_JSON="$LOG_DIR/test_results.json"
TEST_REPORT_HTML="$LOG_DIR/test_report.html"

# Config
MAYU_CONFIG="$PROJECT_ROOT/keymaps/config_clean.mayu"

# Baseline pass rate from task 1.6
BASELINE_PASS_RATE=50.0

# Test timeouts
YAMY_STARTUP_TIMEOUT=5
YAMY_SHUTDOWN_TIMEOUT=5
TEST_TIMEOUT=60

################################################################################
# Global state
################################################################################

YAMY_PID=""
EXIT_CODE=0
CLEANUP_DONE=0

################################################################################
# Utility functions
################################################################################

# Print colored output
print_header() {
    echo ""
    echo "================================================================================"
    echo "$1"
    echo "================================================================================"
    echo ""
}

print_success() {
    echo "[✓] $1"
}

print_error() {
    echo "[✗] $1" >&2
}

print_info() {
    echo "[i] $1"
}

print_warning() {
    echo "[!] $1"
}

# Check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check if YAMY process is running
is_yamy_running() {
    if [ -n "$YAMY_PID" ] && kill -0 "$YAMY_PID" 2>/dev/null; then
        return 0
    fi
    pgrep -x yamy >/dev/null 2>&1
}

################################################################################
# Cleanup handler
################################################################################

cleanup() {
    if [ "$CLEANUP_DONE" -eq 1 ]; then
        return
    fi
    CLEANUP_DONE=1

    print_info "Cleaning up..."

    # Stop YAMY if we started it
    if [ -n "$YAMY_PID" ]; then
        print_info "Stopping YAMY (PID: $YAMY_PID)..."
        if kill -0 "$YAMY_PID" 2>/dev/null; then
            kill "$YAMY_PID" 2>/dev/null || true

            # Wait for graceful shutdown
            local timeout=$YAMY_SHUTDOWN_TIMEOUT
            while [ $timeout -gt 0 ] && kill -0 "$YAMY_PID" 2>/dev/null; do
                sleep 1
                timeout=$((timeout - 1))
            done

            # Force kill if still running
            if kill -0 "$YAMY_PID" 2>/dev/null; then
                print_warning "YAMY did not stop gracefully, forcing..."
                kill -9 "$YAMY_PID" 2>/dev/null || true
            fi

            print_success "YAMY stopped"
        fi
    fi

    # Stop any stray YAMY processes we might have started
    pkill -x yamy 2>/dev/null || true

    # Restore session backup if it exists
    local session_file="$HOME/.config/yamy/session.json"
    local session_backup="$HOME/.config/yamy/session.json.backup"
    if [ -f "$session_backup" ]; then
        mv "$session_backup" "$session_file"
        print_info "Restored session backup"
    fi

    print_info "Cleanup complete"
}

# Trap exit signals
trap cleanup EXIT INT TERM

################################################################################
# Pre-flight checks
################################################################################

preflight_checks() {
    print_header "Pre-flight Checks"

    # Check if running on Linux
    if [ "$(uname -s)" != "Linux" ]; then
        print_error "This script requires Linux"
        return 1
    fi

    # Check required executables
    local required_bins=(
        "$YAMY_BIN:YAMY binary"
        "$YAMY_TEST_BIN:yamy-test utility"
        "$UNIT_TEST_BIN:Unit test binary"
        "$INTEGRATION_TEST_BIN:Integration test binary"
    )

    for entry in "${required_bins[@]}"; do
        local bin="${entry%%:*}"
        local name="${entry#*:}"
        if [ ! -x "$bin" ]; then
            print_error "Missing or not executable: $name ($bin)"
            print_info "Run 'make' or 'ninja' in $BUILD_DIR to build tests"
            return 1
        fi
        print_success "Found $name"
    done

    # Check Python scripts
    local required_scripts=(
        "$AUTOMATED_TEST:Automated test framework"
        "$REPORT_GENERATOR:Report generator"
    )

    for entry in "${required_scripts[@]}"; do
        local script="${entry%%:*}"
        local name="${entry#*:}"
        if [ ! -f "$script" ]; then
            print_error "Missing: $name ($script)"
            return 1
        fi
        print_success "Found $name"
    done

    # Check Python 3
    if ! command_exists python3; then
        print_error "Python 3 is required"
        return 1
    fi
    print_success "Found Python 3: $(python3 --version)"

    # Check config file
    if [ ! -f "$MAYU_CONFIG" ]; then
        print_error "Missing config file: $MAYU_CONFIG"
        return 1
    fi
    print_success "Found config file: $MAYU_CONFIG"

    # Check if YAMY is already running
    if is_yamy_running; then
        print_warning "YAMY is already running"
        print_warning "This script will start its own instance for testing"
        print_info "Consider stopping existing YAMY instances first"
    fi

    print_success "All pre-flight checks passed"
    return 0
}

################################################################################
# YAMY lifecycle management
################################################################################

start_yamy() {
    print_header "Starting YAMY in Test Mode"

    # Clear old log file (YAMY will recreate it)
    rm -f "$YAMY_LOG"

    # Setup session file to load the test config
    local session_file="$HOME/.config/yamy/session.json"
    local session_backup="$HOME/.config/yamy/session.json.backup"

    # Backup existing session if it exists
    if [ -f "$session_file" ]; then
        cp "$session_file" "$session_backup"
        print_info "Backed up existing session file"
    fi

    # Create session directory if it doesn't exist
    mkdir -p "$(dirname "$session_file")"

    # Create test session file pointing to test config with engine running
    cat > "$session_file" << EOF
{
  "activeConfigPath": "$MAYU_CONFIG",
  "engineWasRunning": true,
  "savedTimestamp": $(date +%s),
  "windowPositions": {}
}
EOF
    print_info "Created test session file with config: $MAYU_CONFIG"

    # Start YAMY with debug logging
    # Engine logs (keycode processing) go to stderr
    # GUI logs go to /tmp/yamy-debug.log internally
    print_info "Starting YAMY with debug logging enabled..."
    print_info "Engine log file (stderr): $YAMY_LOG"

    YAMY_DEBUG_KEYCODE=1 "$YAMY_BIN" 2> "$YAMY_LOG" &
    YAMY_PID=$!

    print_info "YAMY PID: $YAMY_PID"

    # Wait for YAMY to be ready
    print_info "Waiting for YAMY to initialize..."
    local timeout=$YAMY_STARTUP_TIMEOUT
    while [ $timeout -gt 0 ]; do
        if ! kill -0 "$YAMY_PID" 2>/dev/null; then
            print_error "YAMY process died during startup"
            print_info "Log output:"
            cat "$YAMY_LOG"

            # Restore session backup
            if [ -f "$session_backup" ]; then
                mv "$session_backup" "$session_file"
                print_info "Restored session backup"
            fi

            return 1
        fi

        # Check if YAMY is ready (log file has content)
        if [ -s "$YAMY_LOG" ]; then
            # Give it a bit more time to fully initialize
            sleep 1
            break
        fi

        sleep 1
        timeout=$((timeout - 1))
    done

    if [ $timeout -eq 0 ]; then
        print_warning "YAMY startup timeout, but process is running"
        print_info "Proceeding anyway..."
    fi

    # Verify YAMY is still running
    if ! kill -0 "$YAMY_PID" 2>/dev/null; then
        print_error "YAMY process not running after startup"

        # Restore session backup
        if [ -f "$session_backup" ]; then
            mv "$session_backup" "$session_file"
            print_info "Restored session backup"
        fi

        return 1
    fi

    print_success "YAMY started successfully"
    return 0
}

################################################################################
# Test execution phases
################################################################################

run_unit_tests() {
    print_header "Phase 1: Unit Tests (C++ GoogleTest)"

    print_info "Running unit tests for EventProcessor layers..."
    print_info "Tests: Layer 1 (evdevToYamy), Layer 2 (substitution), Layer 3 (yamyToEvdev)"

    if "$UNIT_TEST_BIN"; then
        print_success "Unit tests PASSED"
        return 0
    else
        local code=$?
        print_error "Unit tests FAILED (exit code: $code)"
        return 1
    fi
}

run_integration_tests() {
    print_header "Phase 2: Integration Tests (C++ GoogleTest)"

    print_info "Running integration tests for Layer 1→2→3 composition..."
    print_info "Tests: Complete event flow, event type preservation, known substitutions"

    if "$INTEGRATION_TEST_BIN"; then
        print_success "Integration tests PASSED"
        return 0
    else
        local code=$?
        print_error "Integration tests FAILED (exit code: $code)"
        return 1
    fi
}

run_e2e_tests() {
    print_header "Phase 3: E2E Tests (Python Autonomous Framework)"

    print_info "Running end-to-end tests with live YAMY instance..."
    print_info "Tests: All 87 substitutions × 2 event types (PRESS/RELEASE) = 174 tests"
    print_info "Config: $MAYU_CONFIG"
    print_info "Results will be exported to: $TEST_RESULTS_JSON"

    # Verify YAMY is still running
    if ! is_yamy_running; then
        print_error "YAMY is not running!"
        return 1
    fi

    # Run automated tests with JSON export
    if python3 "$AUTOMATED_TEST" \
        --config "$MAYU_CONFIG" \
        --yamy-test "$YAMY_TEST_BIN" \
        --log "$YAMY_LOG" \
        --json "$TEST_RESULTS_JSON"; then
        print_success "E2E tests PASSED"
        return 0
    else
        local code=$?
        print_error "E2E tests FAILED (exit code: $code)"
        return 1
    fi
}

generate_test_report() {
    print_header "Phase 4: Test Report Generation"

    print_info "Generating HTML test report..."
    print_info "Input: $TEST_RESULTS_JSON"
    print_info "Output: $TEST_REPORT_HTML"
    print_info "Baseline: ${BASELINE_PASS_RATE}%"

    if [ ! -f "$TEST_RESULTS_JSON" ]; then
        print_warning "No test results JSON found, skipping report generation"
        return 0
    fi

    if python3 "$REPORT_GENERATOR" \
        --input "$TEST_RESULTS_JSON" \
        --output "$TEST_REPORT_HTML" \
        --baseline "$BASELINE_PASS_RATE"; then
        print_success "Test report generated: $TEST_REPORT_HTML"
        return 0
    else
        local code=$?
        print_warning "Report generation failed (exit code: $code)"
        return 0  # Don't fail on report generation
    fi
}

################################################################################
# Main test orchestration
################################################################################

run_all_tests() {
    local phase_failures=0

    # Phase 1: Unit Tests (C++)
    if ! run_unit_tests; then
        phase_failures=$((phase_failures + 1))
        EXIT_CODE=1
    fi

    # Phase 2: Integration Tests (C++)
    if ! run_integration_tests; then
        phase_failures=$((phase_failures + 1))
        EXIT_CODE=1
    fi

    # Phase 3: E2E Tests (Python) - requires YAMY running
    if ! run_e2e_tests; then
        phase_failures=$((phase_failures + 1))
        EXIT_CODE=1
    fi

    # Phase 4: Generate Report
    generate_test_report

    # Summary
    print_header "Test Summary"

    if [ $phase_failures -eq 0 ]; then
        print_success "All test phases PASSED"
        print_info "Test report: $TEST_REPORT_HTML"
        return 0
    else
        print_error "Test failures detected: $phase_failures phase(s) failed"
        print_info "Test report: $TEST_REPORT_HTML"
        return 1
    fi
}

################################################################################
# Main entry point
################################################################################

main() {
    print_header "YAMY Comprehensive Test Runner"
    print_info "Project: $PROJECT_ROOT"
    print_info "Build: $BUILD_DIR"
    print_info "Logs: $LOG_DIR"

    # Pre-flight checks
    if ! preflight_checks; then
        print_error "Pre-flight checks failed"
        exit 1
    fi

    # Start YAMY
    if ! start_yamy; then
        print_error "Failed to start YAMY"
        exit 1
    fi

    # Run all test phases
    if ! run_all_tests; then
        print_error "Test suite failed"
        exit "$EXIT_CODE"
    fi

    print_header "Test Run Complete"
    print_success "All tests passed successfully!"
    print_info "View detailed report: $TEST_REPORT_HTML"

    exit 0
}

# Run main
main "$@"
