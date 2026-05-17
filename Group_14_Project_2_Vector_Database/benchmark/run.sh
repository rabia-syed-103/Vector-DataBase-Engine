echo "========================================"
echo "   Vector DB Benchmark Runner"
echo "========================================"

cd "$(dirname "$0")/.."

echo "Stopping any existing server on port 5556..."
pkill -f "vdb --" 2>/dev/null
sleep 1

echo "Building..."
make
if [ $? -ne 0 ]; then
    echo "ERROR: build failed"
    exit 1
fi
mkdir -p vdata

echo "Starting server (dim=64, port=5556)..."
./vdb --dim 64 --port 5556 --data ./vdata &
SERVER_PID=$!
sleep 1

if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "ERROR: server failed to start"
    exit 1
fi

echo "Server started (PID=$SERVER_PID)"
echo ""

./benchmark/bench | tee benchmark/results.txt

echo ""
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "Done. Results saved to benchmark/results.txt"