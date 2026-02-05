#!/usr/bin/env python3
import os

# The script decides the format
print("Content-Type: text/html\r\n\r\n", end="")

print("<html>")
print("<body>")
print("<h1>CGI Test Page</h1>")
print("<p>The server passed through my HTML header!</p>")
print("</body>")
print("</html>")