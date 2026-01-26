#!/usr/bin/env python3
import sys

# Python adds one \n per print()
print("Content-Type: text/plain")
print("Status: 200 OK")
print("") # This creates the second \n, making the \n\n separator

# No more prints = Empty Body