#!/usr/bin/env python3
import time
import sys

# Send the header immediately so the server knows the request was accepted
print("Content-Type: text/plain\r\n\r\n", end="")
sys.stdout.flush()

counter = 1
try:
    while True:
        # Print a message every second
        print(f"Cycle {counter}: Still running... I will never stop!")
        sys.stdout.flush() # Force the output to the server
        counter += 1
        time.sleep(1)
except Exception as e:
    # If the server closes the pipe, this might catch it
    with open("cgi_error.log", "a") as f:
        f.write(f"CGI terminated: {str(e)}\n")