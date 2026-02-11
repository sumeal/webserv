#!/bin/bash
# filepath: /home/muzz/core/webserv_git/test_methods.sh

# Colors
GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[0;33m"
CYAN="\033[0;36m"
RESET="\033[0m"

PASS=0
FAIL=0
TOTAL=0

check() {
    local method=$1
    local url=$2
    local expected=$3
    local description=$4
    local extra_args=$5

    TOTAL=$((TOTAL + 1))
    status=$(curl -s -o /dev/null -w "%{http_code}" -X "$method" $extra_args "$url" 2>/dev/null)

    if [ "$status" -eq "$expected" ]; then
        echo -e "  ${GREEN}✅ PASS${RESET} | $method $url -> $status (expected $expected) | $description"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}❌ FAIL${RESET} | $method $url -> $status (expected $expected) | $description"
        FAIL=$((FAIL + 1))
    fi
}

# Check for redirect (3xx) without following
check_redirect() {
    local method=$1
    local url=$2
    local expected_code=$3
    local expected_location=$4
    local description=$5

    TOTAL=$((TOTAL + 1))
    response=$(curl -s -o /dev/null -w "%{http_code} %{redirect_url}" -X "$method" "$url" 2>/dev/null)
    status=$(echo "$response" | awk '{print $1}')
    redirect_url=$(echo "$response" | awk '{print $2}')

    if [ "$status" -eq "$expected_code" ]; then
        echo -e "  ${GREEN}✅ PASS${RESET} | $method $url -> $status (expected $expected_code) | $description"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}❌ FAIL${RESET} | $method $url -> $status (expected $expected_code) | $description"
        FAIL=$((FAIL + 1))
    fi
}

# ============================================================
# Setup test files
# ============================================================
mkdir -p ./www/uploads
mkdir -p ./www/autoindex
mkdir -p ./www/cgi-bin
echo "delete me" > ./www/uploads/deleteme.txt
echo "delete me 2" > ./www/uploads/deleteme2.txt
echo "autoindex test" > ./www/autoindex/testfile.txt

# ============================================================
#  SERVER 1 — port 8088 (client_max_body_size 200)
# ============================================================
HOST1="http://localhost:8088"
echo ""
echo -e "${CYAN}================================================${RESET}"
echo -e "${CYAN}  SERVER 1 — port 8088 (max body: 200)${RESET}"
echo -e "${CYAN}================================================${RESET}"

# --- location / (GET only, autoindex on) ---
echo ""
echo -e "${YELLOW}--- location / (GET only, autoindex on) ---${RESET}"
check "GET"     "$HOST1/"           200  "GET allowed"
check "POST"    "$HOST1/"           405  "POST not allowed"
check "DELETE"  "$HOST1/"           405  "DELETE not allowed"
check "PUT"     "$HOST1/"           501  "PUT not implemented"

# --- location /uploads (GET POST DELETE, autoindex on) ---
echo ""
echo -e "${YELLOW}--- location /uploads (GET POST DELETE) ---${RESET}"
check "GET"     "$HOST1/uploads/"                   200  "GET directory listing"
check "POST"    "$HOST1/uploads/"                   200  "POST small body" "-H 'Content-Type: text/plain' -d 'hello'"
check "DELETE"  "$HOST1/uploads/deleteme.txt"       200  "DELETE existing file"
check "DELETE"  "$HOST1/uploads/nonexistent.txt"    404  "DELETE non-existent file"
check "PUT"     "$HOST1/uploads/"                   501  "PUT not implemented"

# --- location /cgi-bin (GET POST) ---
echo ""
echo -e "${YELLOW}--- location /cgi-bin (GET POST) ---${RESET}"
check "GET"     "$HOST1/cgi-bin/test.py"            200  "GET CGI script"
check "POST"    "$HOST1/cgi-bin/test.py"            200  "POST CGI script" "-H 'Content-Type: text/plain' -d 'name=test'"
check "DELETE"  "$HOST1/cgi-bin/test.py"            405  "DELETE not allowed"
check "PUT"     "$HOST1/cgi-bin/test.py"            501  "PUT not implemented"

# --- Max body size (200 bytes) ---
echo ""
echo -e "${YELLOW}--- Max Body Size (limit: 200 bytes) ---${RESET}"
LARGE_BODY=$(python3 -c "print('A' * 300)")
check "POST"    "$HOST1/uploads/"   413  "Body exceeds limit (300 bytes)" "-H 'Content-Type: text/plain' -d '$LARGE_BODY'"
check "POST"    "$HOST1/uploads/"   200  "Body within limit (5 bytes)" "-H 'Content-Type: text/plain' -d 'hello'"

# --- Edge cases ---
echo ""
echo -e "${YELLOW}--- Edge Cases ---${RESET}"
check "GET"     "$HOST1/nonexistent"            404  "Non-existent path"
check "GET"     "$HOST1/../etc/passwd"          403  "Path traversal blocked"
check "GET"     "$HOST1/..%2F..%2Fetc/passwd"   403  "Encoded path traversal blocked"

# ============================================================
#  SERVER 2 — port 8089 (client_max_body_size 2000000)
# ============================================================
HOST2="http://localhost:8089"
echo ""
echo -e "${CYAN}================================================${RESET}"
echo -e "${CYAN}  SERVER 2 — port 8089 (max body: 2000000)${RESET}"
echo -e "${CYAN}================================================${RESET}"

# --- location / (GET only, autoindex off) ---
echo ""
echo -e "${YELLOW}--- location / (GET only, autoindex off) ---${RESET}"
check "GET"     "$HOST2/"           200  "GET allowed"
check "POST"    "$HOST2/"           405  "POST not allowed"
check "DELETE"  "$HOST2/"           405  "DELETE not allowed"
check "PUT"     "$HOST2/"           501  "PUT not implemented"

# --- location /www (GET only, autoindex off) ---
echo ""
echo -e "${YELLOW}--- location /www (GET only, autoindex off) ---${RESET}"
check "GET"     "$HOST2/www/"       200  "GET allowed"
check "POST"    "$HOST2/www/"       405  "POST not allowed"
check "DELETE"  "$HOST2/www/"       405  "DELETE not allowed"

# --- location /uploads (GET POST DELETE, autoindex off) ---
echo ""
echo -e "${YELLOW}--- location /uploads (GET POST DELETE) ---${RESET}"
check "GET"     "$HOST2/uploads/"                   200  "GET directory (autoindex off)"
check "POST"    "$HOST2/uploads/"                   200  "POST small body" "-H 'Content-Type: text/plain' -d 'hello'"
check "DELETE"  "$HOST2/uploads/deleteme2.txt"      200  "DELETE existing file"
check "PUT"     "$HOST2/uploads/"                   501  "PUT not implemented"

# --- location /autoindex (GET POST DELETE, autoindex on) ---
echo ""
echo -e "${YELLOW}--- location /autoindex (GET POST DELETE, autoindex on) ---${RESET}"
check "GET"     "$HOST2/autoindex/"     200  "GET directory listing"
check "POST"    "$HOST2/autoindex/"     200  "POST allowed" "-H 'Content-Type: text/plain' -d 'test'"
check "DELETE"  "$HOST2/autoindex/"     405  "DELETE directory (no file specified)"
check "PUT"     "$HOST2/autoindex/"     501  "PUT not implemented"

# --- location /cgi-bin (GET POST) ---
echo ""
echo -e "${YELLOW}--- location /cgi-bin (GET POST) ---${RESET}"
check "GET"     "$HOST2/cgi-bin/test.py"            200  "GET CGI script"
check "POST"    "$HOST2/cgi-bin/test.py"            200  "POST CGI script" "-H 'Content-Type: text/plain' -d 'name=test'"
check "DELETE"  "$HOST2/cgi-bin/test.py"            405  "DELETE not allowed"
check "PUT"     "$HOST2/cgi-bin/test.py"            501  "PUT not implemented"

# --- Redirects ---
echo ""
echo -e "${YELLOW}--- Redirects ---${RESET}"
check_redirect "GET" "$HOST2/42kl"      301  "https://42kl.edu.my/"      "301 redirect"
check_redirect "GET" "$HOST2/sci-hub"   302  "https://www.sci-hub.in"    "302 redirect"
check_redirect "GET" "$HOST2/42kl3"     303  "https://42kl.edu.my/"      "303 redirect"

# --- Max body size (2000000 bytes) ---
echo ""
echo -e "${YELLOW}--- Max Body Size (limit: 2000000 bytes) ---${RESET}"
check "POST"    "$HOST2/uploads/"   200  "Large body within limit (1000 bytes)" "-H 'Content-Type: text/plain' -d '$(python3 -c "print('B' * 1000)")'"

# ============================================================
#  SERVER 3 — port 8090 (client_max_body_size 200)
# ============================================================
HOST3="http://localhost:8090"
echo ""
echo -e "${CYAN}================================================${RESET}"
echo -e "${CYAN}  SERVER 3 — port 8090 (max body: 200)${RESET}"
echo -e "${CYAN}================================================${RESET}"

# --- location / (GET only, autoindex on) ---
echo ""
echo -e "${YELLOW}--- location / (GET only, autoindex on) ---${RESET}"
check "GET"     "$HOST3/"           200  "GET allowed (www2 root)"
check "POST"    "$HOST3/"           405  "POST not allowed"
check "DELETE"  "$HOST3/"           405  "DELETE not allowed"
check "PUT"     "$HOST3/"           501  "PUT not implemented"

# --- Edge cases ---
echo ""
echo -e "${YELLOW}--- Edge Cases ---${RESET}"
check "GET"     "$HOST3/nonexistent"    404  "Non-existent path"

# ============================================================
#  Connection / Keep-Alive Tests
# ============================================================
echo ""
echo -e "${CYAN}================================================${RESET}"
echo -e "${CYAN}  Connection Tests${RESET}"
echo -e "${CYAN}================================================${RESET}"

# HTTP/1.1 keep-alive (default)
echo ""
echo -e "${YELLOW}--- Keep-Alive ---${RESET}"
TOTAL=$((TOTAL + 1))
body=$(curl -s -H "Connection: keep-alive" "$HOST1/" 2>/dev/null)
if [ -n "$body" ]; then
    echo -e "  ${GREEN}✅ PASS${RESET} | GET $HOST1/ with Connection: keep-alive | Response received"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}❌ FAIL${RESET} | GET $HOST1/ with Connection: keep-alive | No response"
    FAIL=$((FAIL + 1))
fi

# HTTP/1.1 close
TOTAL=$((TOTAL + 1))
body=$(curl -s -H "Connection: close" "$HOST1/" 2>/dev/null)
if [ -n "$body" ]; then
    echo -e "  ${GREEN}✅ PASS${RESET} | GET $HOST1/ with Connection: close | Response received"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}❌ FAIL${RESET} | GET $HOST1/ with Connection: close | No response"
    FAIL=$((FAIL + 1))
fi

# ============================================================
#  Summary
# ============================================================
echo ""
echo -e "${CYAN}================================================${RESET}"
echo -e "${CYAN}  Results: $PASS/$TOTAL passed, $FAIL failed${RESET}"
echo -e "${CYAN}================================================${RESET}"

if [ "$FAIL" -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${RESET}"
    exit 0
else
    echo -e "${RED}Some tests failed!${RESET}"
    exit 1
fi