#!/usr/bin/env python3
import os
import sys

# 1. Print headers (CGI must provide at least one)
print("Content-Type: text/plain")
print("Status: 200 OK")
print("") # The empty line separating headers from body

# 2. Print Environment Variables (Verifies your envp_vec)
print("--- ENVIRONMENT VARIABLES ---")
for key, value in os.environ.items():
    print(f"{key}: {value}")

# 3. Print Standard Input (Verifies your write to pipe_in)
print("\n--- POST BODY DATA ---")
body = sys.stdin.read()
if body:
    print(f"Body received: {body}")
else:
    print("No body data received.")