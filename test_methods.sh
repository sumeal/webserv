#!/bin/bash

# ========================
# Webserv Full Method Test Script
# ========================

# Helper function to test a method
test_method() {
    local method=$1
    local url=$2
    local data=$3

    echo -e "\n$method $url"
    if [ "$method" == "POST" ]; then
        curl -i -X POST -H "Content-Type: plain/text" --data "$data" "$url"
    elif [ "$method" == "DELETE" ]; then
        curl -i -X DELETE "$url"
    else
        curl -i "$url"
    fi
}

# ------------------------
# Server on port 8088
# ------------------------
echo "===== Testing Server 8088 ====="
BASE_URL_8088="http://localhost:8088"

# Locations to test
LOCATIONS_8088=(
    "/" 
    "/uploads/hello.txt" 
    "/cgi-bin/test.py"
)

# Allowed methods per location
declare -A METHODS_8088
METHODS_8088["/"]=(GET)
METHODS_8088["/uploads/hello.txt"]=(GET POST DELETE)
METHODS_8088["/cgi-bin/test.py"]=(GET POST)

for loc in "${LOCATIONS_8088[@]}"; do
    for method in "${METHODS_8088[$loc][@]}"; do
        data="Test body for $loc"
        test_method "$method" "$BASE_URL_8088$loc" "$data"
    done
done

# ------------------------
# Server on port 8089
# ------------------------
echo -e "\n===== Testing Server 8089 ====="
BASE_URL_8089="http://localhost:8089"

LOCATIONS_8089=(
    "/" 
    "/www/index.html" 
    "/uploads/test2.txt" 
    "/autoindex/" 
    "/cgi-bin/test.py"
)

declare -A METHODS_8089
METHODS_8089["/"]=(GET)
METHODS_8089["/www/index.html"]=(GET)
METHODS_8089["/uploads/test2.txt"]=(GET POST DELETE)
METHODS_8089["/autoindex/"]=(GET)
METHODS_8089["/cgi-bin/test.py"]=(GET POST)

for loc in "${LOCATIONS_8089[@]}"; do
    for method in "${METHODS_8089[$loc][@]}"; do
        data="Test body for $loc"
        test_method "$method" "$BASE_URL_8089$loc" "$data"
    done
done

# Test redirects
REDIRECTS=("/42kl" "/sci-hub" "/42kl3")
for loc in "${REDIRECTS[@]}"; do
    echo -e "\nGET $BASE_URL_8089$loc (redirect)"
    curl -i "$BASE_URL_8089$loc"
done

# ------------------------
# Server on port 8090
# ------------------------
echo -e "\n===== Testing Server 8090 ====="
BASE_URL_8090="http://localhost:8090"

LOCATIONS_8090=("/")

declare -A METHODS_8090
METHODS_8090["/"]=(GET)

for loc in "${LOCATIONS_8090[@]}"; do
    for method in "${METHODS_8090[$loc][@]}"; do
        data="Test body for $loc"
        test_method "$method" "$BASE_URL_8090$loc" "$data"
    done
done

# Test a non-existent page
echo -e "\nGET $BASE_URL_8090/nonexistent (should return 404)"
curl -i "$BASE_URL_8090/nonexistent"
