#!/usr/bin/python3
import os

# Define the filename we want
filename = "config.txt"

# Get the absolute path the script is actually using
# This combines the Current Working Directory + the filename
access_path = os.path.abspath(filename)
cwd = os.getcwd()

try:
    with open(filename, "r") as f:
        print("Content-type: text/html\r\n\r\n")
        print(f"<b>Success!</b><br>")
        print(f"Script CWD: {cwd}<br>")
        print(f"Accessing path: {access_path}<br>")
        print("<hr>")
        print("File content: ", f.read())
except FileNotFoundError:
    # We still need headers so the browser/curl can display the error
    print("Status: 501 Internal Server Error\r\n\r\n")
    print(f"<b>Error: File Not Found</b><br>")
    print(f"Script CWD: {cwd}<br>")
    print(f"Attempted to access: {access_path}<br>")
    print("<br>Check if the file exists at that exact path!")