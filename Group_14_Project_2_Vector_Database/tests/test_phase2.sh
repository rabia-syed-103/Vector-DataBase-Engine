#!/bin/bash

echo "=== Phase 2 Test ==="

# start server in background
./vdb --data ./vdata --dim 4 --port 5556 &
PID=$!
sleep 1

send() {
    echo "$1" | ./vdb-cli localhost 5556
}

echo "--- Adding 5 vectors ---"
send "ADD 1 0.10 0.20 0.30 0.40"
send "ADD 2 0.15 0.25 0.35 0.45"
send "ADD 3 0.90 0.10 0.10 0.20"
send "ADD 4 0.85 0.15 0.05 0.25"
send "ADD 5 0.50 0.50 0.50 0.50"

echo ""
echo "--- BUILD ---"
send "BUILD"

echo ""
echo "--- STATS after BUILD ---"
send "STATS"

echo ""
echo "--- BRUTE SEARCH ---"
send "SEARCH 0.12 0.22 0.32 0.42 3 BRUTE"

echo ""
echo "--- IVF SEARCH nprobe=2 ---"
send "SEARCH 0.12 0.22 0.32 0.42 3 IVF 2"

echo ""
echo "--- IVF SEARCH nprobe=1 ---"
send "SEARCH 0.12 0.22 0.32 0.42 3 IVF 1"

echo ""
echo "--- ADD after BUILD (incremental insert) ---"
send "ADD 6 0.11 0.21 0.31 0.41"
send "STATS"

echo ""
echo "--- QUIT ---"
send "QUIT"

kill $PID
echo "=== Test Done ==="
