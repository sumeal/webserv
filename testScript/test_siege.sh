#!/bin/bash

# --------------------------------------------------------------
# :: Readme ::
# Start up the webserver, then run this script to test
# [file]   siege.conf
# --------------------------------------------------------------

# in siege:
# -b → benchmark

# -c → concurrent users
# How many “virtual users” make requests at the same time.
# Example: -c 10 → 10 users hitting the server simultaneously.

# -r → repetitions
# How many times each user repeats their requests.
# Example: -r 5 → each virtual user makes 5 requests in total.

# -t → time limit

# Colours
GREEN="\033[38;2;168;204;124m"
RED="\033[38;2;191;97;106m"
ORANGE="\033[38;2;255;135;0m";
RESET="\033[0m"

# Configuration
PORT=8000

# Tests
echo "${YELLOW}TEST 1 : simulate max load on server, send requests as fast without delay${RESET}"
echo "EXPECTED: Availability above 99.5%"
echo "----------------------------------------"
siege -b http://localhost:8000/empty.html
echo ""

# echo "${YELLOW}TEST 2 : handle multiple simultaneous users${RESET}"
# echo "EXPECTED: Availability above 99.5%"
# echo "----------------------------------------"
# siege -b -c 10 -t 30S http://localhost:8000/empty.html
# echo ""

# echo "${YELLOW}TEST 3 : detect memory leaks or slow-growing issues${RESET}"
# echo "EXPECTED: Availability above 99.5%"
# echo "----------------------------------------"
# siege -b -c 200 -t 30S http://localhost:8000/empty.html
# echo ""


# manual test
# RUN ./webserv conf/siege.conf

# # TEST 1 : simulate max load on server, send requests as fast without delay
# siege -b http://localhost:8000/empty.html

# # TEST 2 : handle multiple simultaneous users
# siege -b -c 10 -t 30S http://localhost:8000/empty.html

# # TEST 3 : detect memory leaks or slow-growing issues
# siege -b -c 200 -t 30S http://localhost:8000/empty.html
