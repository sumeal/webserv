#!/bin/bash

# Configuration
HOST="localhost"
PORT1="8088"
PORT2="8089"

echo "======================================="
echo "   WEBSERV CONFIGURATION TESTER"
echo "======================================="

test_route() {
    local port=$1
    local method=$2
    local path=$3
    local description=$4

    echo -n "[Port $port] $method $path ($description): "
    
    # Run curl and capture the status code
    response=$(curl -s -o /dev/null -w "%{http_code}" -X $method "http://$HOST:$port$path")
    
    if [ "$response" == "200" ] || [ "$response" == "201" ] || [ "$response" == "301" ] || [ "$response" == "302" ] || [ "$response" == "303" ]; then
        echo -e "\033[0;32m$response SUCCESS\033[0m"
    elif [ "$response" == "405" ]; then
        echo -e "\033[0;33m$response METHOD NOT ALLOWED (Expected)\033[0m"
    else
        echo -e "\033[0;31m$response FAILED\033[0m"
    fi
}

# --- PORT 8088 TESTS ---
echo -e "\n--- Testing Server 8088 ---"
test_route $PORT1 "GET"    "/"             "Root"
test_route $PORT1 "POST"   "/"             "Method Restriction Test"
test_route $PORT1 "GET"    "/uploads"      "Autoindex Test"
test_route $PORT1 "GET"    "/cgi-bin/addParcel.php" "CGI Test"

# --- PORT 8089 TESTS ---
echo -e "\n--- Testing Server 8089 ---"
test_route $PORT2 "GET"    "/42kl"         "Redirection 301"
test_route $PORT2 "GET"    "/sci-hub"      "Redirection 302"
test_route $PORT2 "GET"    "/autoindex"    "Autoindex ON Test"

echo -e "\nDone."