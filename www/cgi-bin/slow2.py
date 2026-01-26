#!/usr/bin/env python3
import time
import sys
import os

# 1. Simulate the "thinking" delay
time.sleep(3)

# 2. Get the Request Method from environment variables
method = os.environ.get("REQUEST_METHOD", "GET").upper()

# 3. Print the required CGI Header
print("Content-Type: text/plain\r\n\r\n", end="")

if method == "GET":
    # Handle GET: Usually reading from QUERY_STRING
    query = os.environ.get("QUERY_STRING", "No query found")
    print(f"GET Request Received!\r\n")
    print(f"Query Data: {query}\r\n")
    print("Message: Sorry I'm late! I was thinking for 3 seconds.")

elif method == "POST":
    # Handle POST: Reading the body from Standard Input (stdin)
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    post_data = sys.stdin.read(content_length)
    
    print(f"POST Request Received!\r\n")
    print(f"Body Content: {post_data}\r\n")
    print("Message: I processed your data after thinking for 3 seconds.")

else:
    print(f"Unsupported Method: {method}")