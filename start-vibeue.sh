#!/usr/bin/env bash
# VibeUE MCP Startup Helper
# =========================
# Starts the Unreal Editor (TestProject) and the VibeUE MCP proxy with the
# env vars that the plugin needs on Linux.
#
# This script is project-relative: it locates the .uproject next to itself,
# so it works unmodified in any copy of this template.
#
# Why this exists:
#   1. The VibeUE plugin uses Windows-style %APPDATA% to locate its tools
#      manifest directory. On Linux, this env var must be set explicitly.
#   2. Without it, the plugin skips writing the manifest and the proxy
#      cannot enumerate the available tools.
#   3. The Unreal Editor on Linux may crash in libcef.so on shutdown
#      (a known CEF cleanup issue). It does NOT affect the MCP workflow
#      as long as you let the editor run long enough (~20s) to export
#      the manifest.
#
# Usage:
#   ./start-vibeue.sh           # Start editor + proxy
#   ./start-vibeue.sh editor    # Start only the editor
#   ./start-vibeue.sh proxy     # Start only the proxy
#   ./start-vibeue.sh stop      # Stop both
#   ./start-vibeue.sh status    # Show current state
#
# After starting the editor, wait ~20 seconds for:
#   "VibeUE API key validated successfully"
#   "Exported 10 tools to .../tools-manifest.json"
# in Saved/Logs/TestProject.log

set -e

# --- Locate project -------------------------------------------------------
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UPROJECT="$(ls "$PROJECT_DIR"/*.uproject 2>/dev/null | head -n 1)"
if [ -z "$UPROJECT" ]; then
    echo "ERROR: no .uproject found in $PROJECT_DIR" >&2
    exit 1
fi
PROJECT_NAME="$(basename "$UPROJECT" .uproject)"

# Engine install (edit if your engine lives elsewhere)
ENGINE_BIN="/mnt/data/UE 5.7.4/Engine/Binaries/Linux/UnrealEditor"

PROXY_SCRIPT="$PROJECT_DIR/Plugins/VibeUE/Content/Python/vibeue-proxy.py"
PROXY_PORT=8089
EDITOR_LOG="/tmp/unreal-editor.log"
PROXY_LOG="/tmp/vibeue-proxy.log"
export APPDATA="/home/cristopulos"

start_editor() {
    if pgrep -f "UnrealEditor.*$PROJECT_NAME" > /dev/null; then
        echo "Editor already running."
        return
    fi
    echo "Starting Unreal Editor ($PROJECT_NAME)..."
    cd "$PROJECT_DIR"
    setsid env APPDATA="$APPDATA" "$ENGINE_BIN" "$UPROJECT" \
        > "$EDITOR_LOG" 2>&1 < /dev/null &
    disown
    echo "Editor launched. PID: $!"
    echo "Waiting 20s for plugin initialization..."
    sleep 20
    echo "Done. Check $PROJECT_DIR/Saved/Logs/$PROJECT_NAME.log for VibeUE status."
}

start_proxy() {
    if pgrep -f "vibeue-proxy.py" > /dev/null; then
        echo "Proxy already running."
        return
    fi
    echo "Starting VibeUE MCP proxy on port $PROXY_PORT..."
    cd "$PROJECT_DIR"
    setsid env APPDATA="$APPDATA" python3 "$PROXY_SCRIPT" \
        > "$PROXY_LOG" 2>&1 < /dev/null &
    disown
    sleep 2
    if pgrep -f "vibeue-proxy.py" > /dev/null; then
        echo "Proxy running. PID: $(pgrep -f 'vibeue-proxy.py')"
        echo "Tools available at: http://127.0.0.1:$PROXY_PORT/mcp"
    else
        echo "Proxy failed to start. Check $PROXY_LOG"
    fi
}

stop_all() {
    echo "Stopping editor and proxy..."
    pkill -9 -f "UnrealEditor.*$PROJECT_NAME" 2>/dev/null || true
    pkill -9 -f "vibeue-proxy.py" 2>/dev/null || true
    sleep 1
    echo "Done."
}

show_status() {
    echo "=== Editor ==="
    if pgrep -f "UnrealEditor.*$PROJECT_NAME" > /dev/null; then
        pgrep -af "UnrealEditor.*$PROJECT_NAME"
    else
        echo "Not running"
    fi
    echo ""
    echo "=== Proxy ==="
    if pgrep -f "vibeue-proxy.py" > /dev/null; then
        pgrep -af "vibeue-proxy.py"
        echo "Endpoint: http://127.0.0.1:$PROXY_PORT/mcp"
    else
        echo "Not running"
    fi
    echo ""
    echo "=== Manifest ==="
    if [ -f "$APPDATA/VibeUE/tools-manifest.json" ]; then
        ls -la "$APPDATA/VibeUE/tools-manifest.json"
        echo "Tools count: $(python3 -c "import json; d=json.load(open('$APPDATA/VibeUE/tools-manifest.json')); print(len(d.get('tools',[])))" 2>/dev/null || echo '?')"
    else
        echo "Manifest not yet created. Start the editor to generate it."
    fi
    echo ""
    echo "=== MCP quick check ==="
    if command -v curl > /dev/null && pgrep -f "vibeue-proxy.py" > /dev/null; then
        curl -s -m 3 -X POST "http://127.0.0.1:$PROXY_PORT/mcp" \
            -H "Content-Type: application/json" \
            -H "Accept: application/json, text/event-stream" \
            -d '{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{}}' \
            | python3 -c "import json,sys; d=json.load(sys.stdin); t=d.get('result',{}).get('tools',[]); print(f'Proxy reports {len(t)} tools')" 2>/dev/null \
            || echo "Could not query proxy"
    else
        echo "Proxy not running"
    fi
}

case "${1:-all}" in
    editor)  start_editor ;;
    proxy)   start_proxy ;;
    stop)    stop_all ;;
    status)  show_status ;;
    all|"")  start_editor; start_proxy; show_status ;;
    *)       echo "Usage: $0 {editor|proxy|all|stop|status}"; exit 1 ;;
esac