#!/usr/bin/python3
# www/cgi-bin/echo_post.py
import sys, os

print("Content-Type: text/plain")
print()

if os.environ.get('REQUEST_METHOD') == 'POST':
    cl = int(os.environ.get('CONTENT_LENGTH', 0))
    if cl > 0:
        data = sys.stdin.read(cl)
        print(f"ECHO: {data}")
    else:
        print("No POST data")
else:
    print("Send me a POST request")