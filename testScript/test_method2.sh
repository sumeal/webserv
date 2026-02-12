#!/bin/bash
# filepath: /home/muzz/core/webserv_git/test_methods.sh

HOST="http://localhost:8088"
PASS=0
FAIL=0
TOTAL=0

# Colors
GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[0;33m"
CYAN="\033[0;36m"
RESET="\033[0m"

check() {
    local method=$1
    local url=$2
    local expected=$3
    local description=$4
    shift 4

    TOTAL=$((TOTAL + 1))
    status=$(curl -s -o /dev/null -w "%{http_code}" -X "$method" "$@" "$url")

    if [ "$status" -eq "$expected" ]; then
        echo -e "  ${GREEN}✅ PASS${RESET} | $method $url -> $status | $description"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}❌ FAIL${RESET} | $method $url -> $status (expected $expected) | $description"
        FAIL=$((FAIL + 1))
    fi
}

# ============================================================
# Setup
# ============================================================
mkdir -p ./www/uploads ./www/cgi-bin
echo "test file content" > ./www/uploads/testfile.txt

echo ""
echo -e "${CYAN}========================================${RESET}"
echo -e "${CYAN}  Webserv Tests — Port 8088${RESET}"
echo -e "${CYAN}========================================${RESET}"

# ============================================================
# location / (GET only)
# ============================================================
echo ""
echo -e "${YELLOW}--- location / (GET only) ---${RESET}"
check "GET"    "$HOST/"      200  "GET allowed"
check "POST"   "$HOST/"      405  "POST not allowed"
check "DELETE" "$HOST/"      405  "DELETE not allowed"
check "PUT"    "$HOST/"      501  "PUT not implemented"

# ============================================================
# location /uploads (GET POST DELETE)
# ============================================================
echo ""
echo -e "${YELLOW}--- location /uploads (GET POST DELETE) ---${RESET}"
check "GET"    "$HOST/uploads/"                200  "GET directory listing"
check "GET"    "$HOST/uploads/testfile.txt"    200  "GET file"
check "POST"   "$HOST/uploads/uploaded.txt"    201  "POST upload file" -H "Content-Type: text/plain" -d "hello world"
check "PUT"    "$HOST/uploads/"                501  "PUT not implemented"

# DELETE tests
echo "delete me" > ./www/uploads/deleteme.txt
check "DELETE" "$HOST/uploads/deleteme.txt"    204  "DELETE existing file"
check "DELETE" "$HOST/uploads/nonexistent.txt" 404  "DELETE non-existent file"

# ============================================================
# location /cgi-bin (GET POST)
# ============================================================
echo ""
echo -e "${YELLOW}--- location /cgi-bin (GET POST) ---${RESET}"
check "GET"    "$HOST/cgi-bin/test.py"     200  "GET CGI script"
check "POST"   "$HOST/cgi-bin/test.py"     200  "POST CGI script" -d "name=test"
check "DELETE" "$HOST/cgi-bin/test.py"     405  "DELETE not allowed"
check "PUT"    "$HOST/cgi-bin/test.py"     501  "PUT not implemented"

# ============================================================
# Max body size (200 bytes limit)
# ============================================================
echo ""
echo -e "${YELLOW}--- Max Body Size (limit: 200 bytes) ---${RESET}"
LARGE_BODY=$(printf 'A%.0s' {1..300})
check "POST"   "$HOST/uploads/"    413  "Body too large (300 bytes)" -H "Content-Type: text/plain" -d "$LARGE_BODY"
check "POST"   "$HOST/uploads/uploaded.txt"    201  "Body within limit (5 bytes)" -H "Content-Type: text/plain" -d "hello"

# ============================================================
# Edge cases
# ============================================================
echo ""
echo -e "${YELLOW}--- Edge Cases ---${RESET}"
check "GET"    "$HOST/nonexistent"          404  "Non-existent path"


# ============================================================
# Summary
# ============================================================
echo ""
echo -e "${CYAN}========================================${RESET}"
echo -e "${CYAN}  Results: $PASS/$TOTAL passed${RESET}"
echo -e "${CYAN}========================================${RESET}"

if [ "$FAIL" -eq 0 ]; then
    echo -e "${GREEN}✅ All tests passed!${RESET}"
    exit 0
else
    echo -e "${RED}❌ $FAIL tests failed!${RESET}"
    exit 1
fi