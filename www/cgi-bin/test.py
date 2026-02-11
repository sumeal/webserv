#!/usr/bin/env python3
# filepath: /home/muzz/core/webserv_git/www/cgi-bin/test.py

import os
import sys

# Read body from stdin if present
body = ""
content_length = os.environ.get("CONTENT_LENGTH", "0")
if content_length and int(content_length) > 0:
    body = sys.stdin.read(int(content_length))

method = os.environ.get("REQUEST_METHOD", "GET")
query = os.environ.get("QUERY_STRING", "")
script = os.environ.get("SCRIPT_NAME", "")
path = os.environ.get("PATH_INFO", "")

html = """<!DOCTYPE html>
<html>
<head><title>CGI Test</title></head>
<body>
<h1>CGI Test Script</h1>
<table border="1">
<tr><td>Method</td><td>{method}</td></tr>
<tr><td>Script</td><td>{script}</td></tr>
<tr><td>Path Info</td><td>{path}</td></tr>
<tr><td>Query String</td><td>{query}</td></tr>
<tr><td>Content Length</td><td>{cl}</td></tr>
<tr><td>Body</td><td>{body}</td></tr>
</table>
</body>
</html>""".format(
    method=method,
    script=script,
    path=path,
    query=query,
    cl=content_length,
    body=body
)

print("Content-Type: text/html")
print("Content-Length: " + str(len(html)))
print("")
print(html)