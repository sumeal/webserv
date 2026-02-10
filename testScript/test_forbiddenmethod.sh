#!/bin/bash

HOST="localhost"
PORT1="8088"
PORT2="8089"

# Color codes
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "======================================="
echo "   METHOD RESTRICTION (ERROR) TESTER"
echo "======================================="

test_forbidden_method() {
    local port=$1
    local path=$2
    local method="PATCH" # Using PATCH because it's never allowed in your config

    echo -n "Checking $method on http://$HOST:$port$path... "
    
    # We use -v to see the headers if we want, but -w captures the code
    response=$(curl -s -o /dev/null -w "%{http_code}" -X $method "http://$HOST:$port$path")
    
    if [ "$response" == "405" ]; then
        echo -e "${RED}Received 405 (Method Not Allowed)${NC}"
    else
        echo -e "Received $response (Check your parser/logic!)"
    fi
}

echo -e "\n--- Testing Server 8088 ---"
test_forbidden_method $PORT1 "/"
test_forbidden_method $PORT1 "/uploads"
test_forbidden_method $PORT1 "/cgi-bin"

echo -e "\n--- Testing Server 8089 ---"
test_forbidden_method $PORT2 "/www"
test_forbidden_method $PORT2 "/"
test_forbidden_method $PORT2 "/uploads"
test_forbidden_method $PORT2 "/autoindex"
test_forbidden_method $PORT2 "/cgi-bin"
test_forbidden_method $PORT2 "/42kl"

echo -e "\nDone. Check your C++ terminal for your print errors!"