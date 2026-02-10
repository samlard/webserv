#!/bin/bash
# Simple test script to create multiple concurrent requests

for i in {1..5}; do
    (curl -s http://localhost:8080/ > /dev/null && echo "Request $i completed") &
done

wait
echo "All requests completed"
