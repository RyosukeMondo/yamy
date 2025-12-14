#!/usr/bin/env bash
set -euo pipefail

# Benchmark YAMY build performance for baseline (GCC + ld) vs modern (Clang + mold).
# Scenarios: clean build, incremental (touch single file), null build.
# Results are printed and saved to logs/build-benchmark-<timestamp>.log.

ROOT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

REPORT_DIR="${ROOT_DIR}/logs"
mkdir -p "$REPORT_DIR"
REPORT_FILE="${REPORT_DIR}/build-benchmark-$(date +%Y%m%d-%H%M%S).log"

JOBS=${JOBS:-$(command -v nproc >/dev/null 2>&1 && nproc || echo 4)}
TIME_CMD=$(command -v /usr/bin/time >/dev/null 2>&1 && echo "/usr/bin/time" || echo "time")

RESULTS=()

log() {
    echo "[$(date +%H:%M:%S)] $*"
}

require_cmd() {
    local cmd="$1"
    if ! command -v "$cmd" >/dev/null 2>&1; then
        log "ERROR: Required command '$cmd' not found in PATH."
        exit 1
    fi
}

print_versions() {
    log "Tool versions:"
    for tool in cmake ninja gcc g++ clang clang++ mold ld.lld ccache; do
        if command -v "$tool" >/dev/null 2>&1; then
            "$tool" --version | head -n 1
        else
            echo "$tool: not found"
        fi
    done
    echo
}

time_build() {
    # Runs build command and returns the real time (seconds) from `time -p`.
    local real
    real=$("$TIME_CMD" -p "$@" 2>&1 >/dev/null | awk '/^real/ {print $2}')
    echo "$real"
}

configure_and_warmup() {
    local build_dir="$1"; shift
    local args=("$@")

    log "Configuring in ${build_dir}..."
    cmake -S "$ROOT_DIR" -B "$build_dir" "${args[@]}"

    log "Warmup build (not measured)..."
    cmake --build "$build_dir" -- -j"$JOBS" >/dev/null
}

measure_suite() {
    local name="$1"; shift
    local build_dir="$1"; shift
    local args=("$@")

    log "=== ${name^^} TOOLCHAIN ==="
    configure_and_warmup "$build_dir" "${args[@]}"

    log "Measuring clean build..."
    local clean_time
    clean_time=$(time_build cmake --build "$build_dir" --clean-first -- -j"$JOBS")

    log "Measuring null build..."
    local null_time
    null_time=$(time_build cmake --build "$build_dir" -- -j"$JOBS")

    local touch_target="${ROOT_DIR}/src/app/main.cpp"
    if [[ ! -f "$touch_target" ]]; then
        log "ERROR: Expected incremental target $touch_target not found."
        exit 1
    fi

    log "Measuring incremental build (touch ${touch_target#${ROOT_DIR}/})..."
    touch "$touch_target"
    local incremental_time
    incremental_time=$(time_build cmake --build "$build_dir" -- -j"$JOBS")

    log "Results for ${name}: clean=${clean_time}s, incremental=${incremental_time}s, null=${null_time}s"
    RESULTS+=("name=${name},clean=${clean_time},incremental=${incremental_time},null=${null_time}")
}

main() {
    require_cmd cmake
    require_cmd ninja
    require_cmd gcc
    require_cmd g++
    require_cmd clang
    require_cmd clang++

    print_versions | tee -a "$REPORT_FILE"

    # Baseline: GCC + ld (force mold/LLD off)
    BASE_BUILD="${ROOT_DIR}/build/bench-baseline"
    measure_suite "baseline" "$BASE_BUILD" \
        -G Ninja \
        -DCMAKE_C_COMPILER=gcc \
        -DCMAKE_CXX_COMPILER=g++ \
        -DMOLD_LINKER=MOLD_LINKER-NOTFOUND \
        -DLLD_LINKER=LLD_LINKER-NOTFOUND | tee -a "$REPORT_FILE"

    # Modern: Clang + mold (auto-detected in CMake)
    MODERN_BUILD="${ROOT_DIR}/build/bench-modern"
    measure_suite "modern" "$MODERN_BUILD" \
        -G Ninja \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ | tee -a "$REPORT_FILE"

    echo >> "$REPORT_FILE"
    echo "SUMMARY:" | tee -a "$REPORT_FILE"
    for entry in "${RESULTS[@]}"; do
        echo "  $entry" | tee -a "$REPORT_FILE"
    done

    log "Benchmark complete. Detailed log: ${REPORT_FILE}"
}

main "$@"
