#!/usr/bin/env python3
import time
import os
import json
import sys

# Simulate slow processing
time.sleep(3)  # 3 seconds delay

# Get request method from environment
method = os.environ.get('REQUEST_METHOD', '')

if method == 'DELETE':
    # Get file path from query string or path info
    path_info = os.environ.get('PATH_INFO', '')
    query_string = os.environ.get('QUERY_SENT')
    
    if path_info:
        file_to_delete = '.' + path_info  # or use your server's root
    elif query_string:
        file_to_delete = query_string.split('=')[1]
    else:
        file_to_delete = 'default.txt'
    
    try:
        if os.path.exists(file_to_delete):
            os.remove(file_to_delete)
            print("Status: 200 OK")
            print("Content-Type: text/plain")
            print()
            print(f"Deleted: {file_to_delete}")
        else:
            print("Status: 404 Not Found")
            print("Content-Type: text/plain")
            print()
            print(f"File not found: {file_to_delete}")
    except Exception as e:
        print("Status: 500 Internal Server Error")
        print("Content-Type: text/plain")
        print()
        print(f"Error: {str(e)}")
else:
    print("Status: 405 Method Not Allowed")
    print("Content-Type: text/plain")
    print()
    print(f"Method {method} not allowed")