#!/usr/bin/env bash
# Debug IPC communication between the GUI client and daemon.
# Provides two modes:
#   1) Proxy mode (default): creates a monitoring proxy socket that logs all traffic.
#   2) Attach mode (--attach): opens an interactive client session to the socket.
# Verbose hexdumps are enabled when YAMY_DEBUG_IPC=1.

set -euo pipefail

DEFAULT_SOCKET_CANDIDATES=(
  "/tmp/yamy-yamy-engine-$(id -u)"
  "/tmp/yamy-engine.sock"
)

SOCKET_PATH=""
PROXY_SOCKET=""
MODE="proxy"
LOG_DIR="${LOG_DIR:-logs}"
LOG_FILE=""
DEBUG="${YAMY_DEBUG_IPC:-0}"

usage() {
  cat <<'EOF'
Usage:
  YAMY_DEBUG_IPC=1 tools/debug_ipc_communication.sh [--socket PATH] [--proxy-socket PATH]
  tools/debug_ipc_communication.sh --attach [--socket PATH]

Options:
  --socket PATH        Path to the Unix socket (auto-detected if omitted).
  --proxy-socket PATH  Proxy socket to expose in proxy mode (defaults to <socket>.debug).
  --attach             Open an interactive client to the socket instead of proxying.
  --help               Show this help.

Notes:
  - Proxy mode keeps the original socket untouched. Point the GUI/clients to the proxy
    socket to capture bidirectional traffic (e.g., export YAMY_IPC_SOCKET=/tmp/yamy-engine.sock.debug).
  - Set YAMY_DEBUG_IPC=1 for hexdump-level verbosity in the logs.
  - Logs are written to $LOG_DIR/ipc_debug_<timestamp>.log and streamed to stdout.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --socket)
      SOCKET_PATH="${2:-}"
      shift 2
      ;;
    --proxy-socket)
      PROXY_SOCKET="${2:-}"
      shift 2
      ;;
    --attach)
      MODE="attach"
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if ! command -v socat >/dev/null 2>&1; then
  echo "socat is required but not installed. Please install socat." >&2
  exit 1
fi

if [[ -z "$SOCKET_PATH" ]]; then
  for candidate in "${DEFAULT_SOCKET_CANDIDATES[@]}"; do
    if [[ -S "$candidate" ]]; then
      SOCKET_PATH="$candidate"
      break
    fi
  done
fi

# Fall back to first candidate even if it does not exist yet.
SOCKET_PATH="${SOCKET_PATH:-${DEFAULT_SOCKET_CANDIDATES[0]}}"

if [[ -z "$PROXY_SOCKET" ]]; then
  PROXY_SOCKET="${SOCKET_PATH}.debug"
fi

mkdir -p "$LOG_DIR"
LOG_FILE="${LOG_FILE:-${LOG_DIR}/ipc_debug_$(date +%Y%m%d_%H%M%S).log}"

timestamp() {
  date "+%Y-%m-%d %H:%M:%S"
}

log_stream() {
  while IFS= read -r line; do
    printf "%s %s\n" "$(timestamp)" "$line" | tee -a "$LOG_FILE"
  done
}

socat_flags=("-t0" "-v")
if [[ "$DEBUG" == "1" ]]; then
  socat_flags=("-t0" "-x" "-v")
fi

if [[ "$MODE" == "proxy" ]]; then
  echo "[IPC DEBUG] Starting proxy with logging -> $LOG_FILE"
  echo "[IPC DEBUG] Forwarding proxy socket: $PROXY_SOCKET"
  echo "[IPC DEBUG] Target socket:          $SOCKET_PATH"
  echo "[IPC DEBUG] Point GUI/clients to proxy socket to capture traffic."
  if [[ -S "$PROXY_SOCKET" ]]; then
    rm -f "$PROXY_SOCKET"
  fi
  exec socat "${socat_flags[@]}" UNIX-LISTEN:"$PROXY_SOCKET",fork,unlink-close,mode=0600 UNIX-CONNECT:"$SOCKET_PATH" \
    2> >(log_stream)
else
  echo "[IPC DEBUG] Attaching interactive client to $SOCKET_PATH"
  echo "[IPC DEBUG] Logging to $LOG_FILE"
  exec socat "${socat_flags[@]}" - UNIX-CONNECT:"$SOCKET_PATH" 2> >(log_stream)
fi
