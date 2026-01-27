#!/usr/bin/env python3
import cgi
import os

# Directory where files will be saved
UPLOAD_DIR = "../uploads"  # relative to CGI script

# Make sure the upload directory exists
os.makedirs(UPLOAD_DIR, exist_ok=True)

# Create FieldStorage instance to parse POST data
form = cgi.FieldStorage()

# Get the file item from the form (name="myfile")
fileitem = form['myfile'] if 'myfile' in form else None

# Debug: print all keys in form
print("Content-Type: text/plain\n")  # print as plain text to see easily
print("DEBUG: form keys:", list(form.keys()))

# Print detailed info about each field
for key in form.keys():
    item = form[key]
    print(f"Field name: {key}")
    print(f"  filename: {getattr(item, 'filename', None)}")
    print(f"  type: {type(item)}")

# Start response
print("Content-Type: text/html\n")
print("<html><head><title>Upload Result</title></head><body>")

if fileitem and fileitem.filename:
    # Extract just the filename (avoid full path)
    filename = os.path.basename(fileitem.filename)

    # Create file path
    filepath = os.path.join(UPLOAD_DIR, filename)

    # Save the file content
    with open(filepath, 'wb') as f:
        f.write(fileitem.file.read())

    print(f"<h1>File '{filename}' uploaded successfully!</h1>")
else:
    print("<h1>No file uploaded!</h1>")

print("</body></html>")
