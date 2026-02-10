#!/bin/bash

# WebServ Test Suite
# Comprehensive testing of all HTTP server features

echo "========================================"
echo "WebServ C++98 HTTP Server - Test Suite"
echo "========================================"
echo

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0

# Test function
test_feature() {
    local name="$1"
    local command="$2"
    local expected="$3"
    
    echo -n "Testing $name... "
    
    result=$(eval "$command" 2>&1)
    
    if echo "$result" | grep -q "$expected"; then
        echo -e "${GREEN}PASS${NC}"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        echo "  Expected: $expected"
        echo "  Got: $result"
        ((TESTS_FAILED++))
    fi
}

echo "1. Testing GET requests"
echo "-----------------------"
test_feature "GET index.html" \
    "curl -s http://localhost:8080/" \
    "WebServ - C++98 HTTP Server"

test_feature "GET text file" \
    "curl -s http://localhost:8080/test.txt" \
    "WebServ C++98 HTTP Server is working"

test_feature "GET 404 error" \
    "curl -s http://localhost:8080/nonexistent.txt" \
    "Error 404"

echo
echo "2. Testing POST requests (file upload)"
echo "---------------------------------------"
test_feature "POST file upload" \
    "curl -X POST -d 'Test content' http://localhost:8080/uploads/test.txt" \
    "File uploaded successfully"

test_feature "Verify uploaded file" \
    "curl -s http://localhost:8080/uploads/test.txt" \
    "Test content"

echo
echo "3. Testing DELETE requests"
echo "--------------------------"
test_feature "DELETE uploaded file" \
    "curl -s -o /dev/null -w '%{http_code}' -X DELETE http://localhost:8080/uploads/test.txt" \
    "204"

test_feature "Verify file deleted" \
    "curl -s http://localhost:8080/uploads/test.txt" \
    "Error 404"

echo
echo "4. Testing CGI execution"
echo "------------------------"
test_feature "CGI without query" \
    "curl -s http://localhost:8080/test.py" \
    "CGI Test"

test_feature "CGI with query string" \
    "curl -s 'http://localhost:8080/test.py?name=Test&value=123'" \
    "Query String: name=Test"

echo
echo "5. Testing Multi-port support"
echo "------------------------------"
test_feature "Port 8080" \
    "curl -s http://localhost:8080/ | grep -c '<title>'" \
    "1"

test_feature "Port 8081" \
    "curl -s http://localhost:8081/ | grep -c '<title>'" \
    "1"

echo
echo "6. Testing Directory Listing"
echo "-----------------------------"
test_feature "Autoindex enabled" \
    "curl -s http://localhost:8080/uploads/" \
    "Index of /uploads/"

echo
echo "7. Testing HTTP methods"
echo "-----------------------"
test_feature "GET method" \
    "curl -s -X GET http://localhost:8080/test.txt | head -1" \
    "This is a test file"

test_feature "POST method" \
    "curl -s -X POST -d 'data' http://localhost:8080/uploads/method-test.txt" \
    "File uploaded"

test_feature "DELETE method" \
    "curl -s -o /dev/null -w '%{http_code}' -X DELETE http://localhost:8080/uploads/method-test.txt" \
    "204"

echo
echo "8. Testing Error Handling"
echo "-------------------------"
test_feature "Invalid method" \
    "curl -s -X INVALID http://localhost:8080/" \
    "Error 405"

echo
echo "========================================"
echo "Test Results Summary"
echo "========================================"
echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"
echo "Total Tests: $((TESTS_PASSED + TESTS_FAILED))"
echo

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi
