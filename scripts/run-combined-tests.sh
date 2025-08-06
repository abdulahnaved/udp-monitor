#!/usr/bin/env bash
# run-combined-tests.sh
# Combined Lane-Switching + JSON Logging Test for udp-monitor

set -euo pipefail

# Where this script lives
SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINDIR="$SCRIPTDIR/../build"
LOGDIR="$SCRIPTDIR/logs"

mkdir -p "$LOGDIR"

echo "=== UDP Monitor Combined Test ==="
echo

# -------------------------------------------------------------------
# Phase 1: Lane‐Switching Test
# -------------------------------------------------------------------
echo "--- Phase 1: Lane‐Switching Test ---"
echo "Cleaning up any leftover processes…"
pkill udp-monitor-server udp-monitor-client 2>/dev/null || true
sleep 1

echo "1) Starting server on Green lane (port 5000, verbose)…"
"$BINDIR/udp-monitor-server" --door 5000 --verbose \
    > "$LOGDIR/lane-server.log" 2>&1 &
SERVER_PID=$!
sleep 2

echo "2) Starting client on Green lane (port 5000)…"
"$BINDIR/udp-monitor-client" --door 5000 \
    > "$LOGDIR/lane-client.log" 2>&1 &
CLIENT_PID=$!
sleep 2

echo "3) Letting them run for 30 s to trigger lane-switches…"
timeout 30s bash -c '
  while kill -0 '"$SERVER_PID"' '"$CLIENT_PID"' >/dev/null 2>&1; do
    sleep 1
  done
' || true

echo "4) Shutting down phase 1…"
kill $CLIENT_PID $SERVER_PID 2>/dev/null || true
wait 2>/dev/null || true

echo
echo "→ Lane Server Log (last 10 lines):"
tail -n 10 "$LOGDIR/lane-server.log" || true

echo
echo "→ Lane Client Log (last 10 lines):"
tail -n 10 "$LOGDIR/lane-client.log" || true

echo
# -------------------------------------------------------------------
# Phase 2: JSON Logging Demo
# -------------------------------------------------------------------
echo "--- Phase 2: JSON-Logging Demo ---"
echo "Cleaning up…"
pkill udp-monitor-server udp-monitor-client 2>/dev/null || true
sleep 1

echo "1) Starting server (port 5000) — note: no --json flag on server"
"$BINDIR/udp-monitor-server" --door 5000 --verbose \
    > "$LOGDIR/server-json.log" 2>&1 &
SERVER_J_PID=$!
sleep 2

echo "2) Starting client with JSON logging on port 5000…"
"$BINDIR/udp-monitor-client" --door 5000 --json \
    > "$LOGDIR/client-json.log" 2>&1 &
CLIENT_J_PID=$!
sleep 2

echo "3) Collecting JSON logs for 15 s…"
sleep 15

echo "4) Shutting down phase 2…"
kill $CLIENT_J_PID $SERVER_J_PID 2>/dev/null || true
wait 2>/dev/null || true

echo
echo "→ Server stdout (last 10 lines):"
tail -n 10 "$LOGDIR/server-json.log" || true

echo
echo "→ Client JSON log (last 10 entries):"
grep '^{' "$LOGDIR/client-json.log" \
  | tail -n 10 \
  | jq . 2>/dev/null || tail -n 10 "$LOGDIR/client-json.log"

echo
echo "=== Combined Test Complete ==="
echo "Logs have been written to: $LOGDIR"
