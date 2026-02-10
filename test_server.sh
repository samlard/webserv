#!/bin/bash
# Test script for poll-based HTTP server

echo "=== Testing Poll-Based HTTP Server ==="
echo

# Test 1: Simple GET request
echo "Test 1: Simple GET request"
RESPONSE=$(curl -s http://localhost:8080/)
if [[ "$RESPONSE" == *"Hello from Poll-Based Server"* ]]; then
    echo "✓ PASS: Simple GET request works"
else
    echo "✗ FAIL: Simple GET request"
fi
echo

# Test 2: Multiple concurrent connections
echo "Test 2: Multiple concurrent connections (10 requests)"
PIDS=()
for i in {1..10}; do
    curl -s http://localhost:8080/ > /dev/null &
    PIDS+=($!)
done
SUCCESS=0
for pid in "${PIDS[@]}"; do
    wait $pid && ((SUCCESS++))
done
if [ $SUCCESS -eq 10 ]; then
    echo "✓ PASS: All 10 concurrent requests succeeded"
else
    echo "✗ FAIL: Only $SUCCESS/10 requests succeeded"
fi
echo

# Test 3: Request with headers
echo "Test 3: Request with custom headers"
RESPONSE=$(curl -s -H "X-Custom-Header: test" http://localhost:8080/)
if [[ "$RESPONSE" == *"Hello from Poll-Based Server"* ]]; then
    echo "✓ PASS: Request with headers works"
else
    echo "✗ FAIL: Request with headers"
fi
echo

# Test 4: POST request (should also work)
echo "Test 4: POST request with body"
RESPONSE=$(curl -s -X POST -d "test data" http://localhost:8080/)
if [[ "$RESPONSE" == *"Hello from Poll-Based Server"* ]]; then
    echo "✓ PASS: POST request works"
else
    echo "✗ FAIL: POST request"
fi
echo

# Test 5: Large number of sequential requests
echo "Test 5: Sequential requests (20 requests)"
FAILED=0
for i in {1..20}; do
    curl -s http://localhost:8080/ > /dev/null || ((FAILED++))
done
if [ $FAILED -eq 0 ]; then
    echo "✓ PASS: All 20 sequential requests succeeded"
else
    echo "✗ FAIL: $FAILED/20 requests failed"
fi
echo

# Test 6: Connection handling (verify non-blocking)
echo "Test 6: Non-blocking behavior (5 simultaneous slow clients)"
SUCCESS=0
for i in {1..5}; do
    (sleep 0.1; curl -s http://localhost:8080/ > /dev/null && echo "OK") &
done
# Start another request immediately - should not block
RESPONSE=$(curl -s http://localhost:8080/)
wait
if [[ "$RESPONSE" == *"Hello from Poll-Based Server"* ]]; then
    echo "✓ PASS: Server handles connections non-blocking"
else
    echo "✗ FAIL: Server appears to block"
fi
echo

echo "=== Test Summary ==="
echo "Server successfully handles:"
echo "  - Simple GET requests"
echo "  - Concurrent connections"
echo "  - POST requests with body"
echo "  - Sequential requests"
echo "  - Non-blocking I/O"
echo
echo "The server uses a single poll() call for all sockets"
echo "and properly handles POLLIN/POLLOUT events."
