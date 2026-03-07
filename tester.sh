#!/bin/bash
# ============================================================
#  tester.sh — Webserv functional test suite
#  Usage:  ./tester.sh [config_file]
#  Default config: config/multi_port.conf  (port 9090 / 9091)
# ============================================================

# Always run from the directory that contains this script (repo root)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# ---------- colours -----------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# ---------- configuration -----------------------------------
BASE_CONF="${1:-config/multi_port.conf}"
BINARY="./webserv"
SERVER_PID=""

PASS=0
FAIL=0
TOTAL=0

# ---------- resolve HOST / HOST2 from config ----------------
# Read ports from the first two 'listen' directives in the config
PORT1=$(grep -m1 'listen' "$BASE_CONF" 2>/dev/null | grep -oE '[0-9]+' | head -1)
PORT2=$(grep    'listen' "$BASE_CONF" 2>/dev/null | grep -oE '[0-9]+' | sed -n '2p')
PORT1="${PORT1:-9090}"
PORT2="${PORT2:-9091}"
HOST="http://localhost:${PORT1}"
HOST2="http://localhost:${PORT2}"

# ---------- resolve bash interpreter -------------------------
# The config may specify cgi_path /usr/bin/bash or /bin/bash.
# Detect what is actually available and patch a temp config copy.
REAL_BASH="$(command -v bash 2>/dev/null)"
if [ -z "$REAL_BASH" ]; then
    printf "${RED}bash not found in PATH — cannot run CGI tests${NC}\n"
    exit 1
fi
CONF="/tmp/webserv_tester_$$.conf"
sed "s|cgi_path [^ ;]*|cgi_path ${REAL_BASH}|g" "$BASE_CONF" > "$CONF"

# ---------- helpers -----------------------------------------
pass() {
    PASS=$((PASS + 1))
    TOTAL=$((TOTAL + 1))
    printf "  ${GREEN}[PASS]${NC} %s\n" "$1"
}

fail() {
    FAIL=$((FAIL + 1))
    TOTAL=$((TOTAL + 1))
    printf "  ${RED}[FAIL]${NC} %s\n" "$1"
    if [ -n "$2" ]; then
        printf "         ${YELLOW}expected:${NC} %s\n" "$2"
        printf "         ${YELLOW}got:     ${NC} %s\n" "$3"
    fi
}

section() {
    printf "\n${CYAN}${BOLD}==> %s${NC}\n" "$1"
}

# assert HTTP status code
assert_status() {
    local label="$1"
    local expected="$2"
    local url="$3"
    shift 3
    local got
    got=$(curl -s -o /dev/null -w "%{http_code}" "$@" "$url")
    if [ "$got" = "$expected" ]; then
        pass "$label (HTTP $expected)"
    else
        fail "$label" "HTTP $expected" "HTTP $got"
    fi
}

# assert response body contains a string
assert_body_contains() {
    local label="$1"
    local needle="$2"
    local url="$3"
    shift 3
    local body
    body=$(curl -s "$@" "$url")
    if echo "$body" | grep -q "$needle"; then
        pass "$label"
    else
        fail "$label" "body contains '$needle'" "body='$(echo "$body" | head -1)'"
    fi
}

# assert response header contains a string (uses GET, not HEAD, to avoid 405)
assert_header_contains() {
    local label="$1"
    local needle="$2"
    local url="$3"
    shift 3
    local headers
    headers=$(curl -s -D - -o /dev/null "$@" "$url")
    if echo "$headers" | grep -qi "$needle"; then
        pass "$label"
    else
        fail "$label" "header contains '$needle'" "headers=$(echo "$headers" | head -5 | tr '\n' '|')"
    fi
}

# ---------- setup -------------------------------------------
start_server() {
    # Always rebuild from latest source
    printf "${YELLOW}Building…${NC}\n"
    make -s
    if [ $? -ne 0 ]; then
        printf "${RED}Build failed. Aborting.${NC}\n"
        exit 1
    fi

    # Kill any leftover processes on our ports (lsof may return multiple PIDs)
    for port in "$PORT1" "$PORT2"; do
        pids=$(lsof -ti tcp:"$port" 2>/dev/null)
        for pid in $pids; do
            kill -9 "$pid" 2>/dev/null || true
        done
    done
    sleep 1

    "$BINARY" "$CONF" > /tmp/webserv_test.log 2>&1 &
    SERVER_PID=$!

    # Wait until the server actually accepts connections (up to 10 s)
    local ready=0
    for i in $(seq 1 20); do
        if curl -s --connect-timeout 1 -o /dev/null "$HOST/" 2>/dev/null; then
            ready=1
            break
        fi
        sleep 0.5
    done

    if [ "$ready" -eq 0 ]; then
        printf "${RED}Server did not become ready. Log:${NC}\n"
        cat /tmp/webserv_test.log
        exit 1
    fi
    printf "${GREEN}Server started (PID %d) with %s${NC}\n" "$SERVER_PID" "$CONF"
}

stop_server() {
    if [ -n "$SERVER_PID" ]; then
        kill -9 "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
        SERVER_PID=""
    fi
    rm -f "$CONF"
}

trap 'stop_server' EXIT INT TERM

# ---------- prepare test data --------------------------------
prepare() {
    # Ensure upload directory exists
    mkdir -p www/uploads

    # Create a fresh delete-target file
    echo "deleteme" > www/test_dir/delete_me.txt
}

# ---------- run tests ----------------------------------------
main() {
    printf "${BOLD}WebServ Tester${NC} — %s\n" "$(date)"
    printf "Config : %s (patched from %s)\n" "$CONF" "$BASE_CONF"
    printf "Host   : %s   Host2: %s\n" "$HOST" "$HOST2"
    printf "Bash   : %s\n" "$REAL_BASH"

    start_server
    prepare

    # --------------------------------------------------------
    section "1. Static file serving"
    assert_status      "GET / returns 200"           200 "$HOST/"
    assert_body_contains "GET / returns HTML body"   "<html"  "$HOST/"
    assert_status      "GET /hello.html returns 200" 200 "$HOST/hello.html"
    assert_status      "GET /index.html returns 200" 200 "$HOST/index.html"

    # --------------------------------------------------------
    section "2. MIME types"
    assert_header_contains "index.html has text/html Content-Type" \
        "text/html" "$HOST/index.html"
    assert_header_contains "Response has Content-Length header" \
        "Content-Length" "$HOST/index.html"

    # --------------------------------------------------------
    section "3. Error pages"
    assert_status "GET /nonexistent returns 404"  404 "$HOST/nonexistent"
    assert_body_contains "404 body mentions 404"  "404" "$HOST/nonexistent"

    # --------------------------------------------------------
    section "4. Method enforcement"
    # Port 2 (9091 by default) only allows GET on /
    assert_status "DELETE on GET-only location returns 405" \
        405 "$HOST2/" -X DELETE
    assert_status "POST on GET-only location returns 405" \
        405 "$HOST2/" -X POST -d "data"

    # --------------------------------------------------------
    section "5. POST — file upload"
    STATUS=$(curl -s -o /dev/null -w "%{http_code}" \
        -X POST --data-binary "hello upload" "$HOST/uploads/")
    if [ "$STATUS" = "201" ]; then
        pass "POST /uploads/ returns 201 Created"
    else
        fail "POST /uploads/ returns 201 Created" "201" "$STATUS"
    fi

    # --------------------------------------------------------
    section "6. DELETE"
    assert_status "DELETE /test_dir/delete_me.txt returns 200" \
        200 "$HOST/test_dir/delete_me.txt" -X DELETE
    # Verify file is gone
    if [ ! -f www/test_dir/delete_me.txt ]; then
        pass "File actually removed from filesystem"
    else
        fail "File actually removed from filesystem" "file absent" "file still present"
    fi

    # --------------------------------------------------------
    section "7. Autoindex"
    assert_status      "GET /test_dir/ returns 200 (autoindex)" \
        200 "$HOST/test_dir/"
    assert_body_contains "Autoindex lists directory entries" \
        "file2.txt" "$HOST/test_dir/"

    # --------------------------------------------------------
    section "8. CGI execution"
    assert_status "GET /cgi-bin/test.sh returns 200" \
        200 "$HOST/cgi-bin/test.sh"
    assert_body_contains "CGI outputs REQUEST_METHOD=GET" \
        "GET" "$HOST/cgi-bin/test.sh"
    assert_body_contains "CGI receives QUERY_STRING" \
        "foo=bar" "$HOST/cgi-bin/test.sh?foo=bar"

    # --------------------------------------------------------
    section "9. client_max_body_size enforcement"
    # Config sets 1 MB limit; send ~1.1 MB
    BIG_BODY=$(python3 -c "print('x' * 1100000)")
    STATUS=$(echo "$BIG_BODY" | curl -s -o /dev/null -w "%{http_code}" \
        -X POST --data-binary @- "$HOST/uploads/")
    if [ "$STATUS" = "413" ]; then
        pass "Oversized POST returns 413 Content Too Large"
    else
        fail "Oversized POST returns 413 Content Too Large" "413" "$STATUS"
    fi

    # --------------------------------------------------------
    section "10. HTTP status line correctness"
    STATUS_LINE=$(curl -s -D - -o /dev/null "$HOST/" | head -1)
    if echo "$STATUS_LINE" | grep -q "200 OK"; then
        pass "Status line includes reason phrase 'OK'"
    else
        fail "Status line includes reason phrase" "HTTP/1.1 200 OK" "$STATUS_LINE"
    fi

    STATUS_LINE404=$(curl -s -D - -o /dev/null "$HOST/nope" | head -1)
    if echo "$STATUS_LINE404" | grep -q "404 Not Found"; then
        pass "404 status line includes 'Not Found'"
    else
        fail "404 status line includes 'Not Found'" "HTTP/1.1 404 Not Found" "$STATUS_LINE404"
    fi

    # --------------------------------------------------------
    section "11. Multiple virtual servers"
    assert_status "Second server (port ${PORT2}) GET / returns 200" \
        200 "$HOST2/"

    # --------------------------------------------------------
    # Summary
    printf "\n${BOLD}========================================${NC}\n"
    printf "${BOLD}Results: %d / %d tests passed${NC}\n" "$PASS" "$TOTAL"
    if [ "$FAIL" -gt 0 ]; then
        printf "${RED}${BOLD}%d test(s) FAILED${NC}\n" "$FAIL"
    else
        printf "${GREEN}${BOLD}All tests passed!${NC}\n"
    fi
    printf "${BOLD}========================================${NC}\n"

    [ "$FAIL" -eq 0 ]
}

main
