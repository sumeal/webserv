#!/usr/bin/python3
import os
import cgi
import cgitb

cgitb.enable()

# Ensure this path is correct and accessible by the web server user
TMP_DIR = "./../uploads"

method = os.environ.get("REQUEST_METHOD", "GET")

print("Content-Type: text/html; charset=utf-8\n")

# --- Logic: Get File List ---
try:
    # Get only filenames, ignore directories
    files = [f for f in os.listdir(TMP_DIR) if os.path.isfile(os.path.join(TMP_DIR, f))]
except Exception:
    files = []

# --- HTML Header ---
print("""
<html>
<head>
    <style>
        body { font-family: Arial, sans-serif; padding: 30px; background: #f5f5f5; }
        .container { max-width: 600px; margin: auto; background: white; padding: 20px; border-radius: 15px; box-shadow: 0 5px 15px rgba(0,0,0,0.2); }
        select, button { width: 100%; padding: 12px; margin: 10px 0; font-size: 16px; }
        button { background: #e74c3c; color: white; border: none; border-radius: 10px; cursor: pointer; }
    </style>
</head>
<body>
<div class="container">
""")

# --- Handle POST (Deletion) ---
if method == "POST":
    form = cgi.FieldStorage()
    filename = form.getfirst("file", "")
    
    # Security: Use os.path.basename to prevent Directory Traversal attacks
    if filename:
        safe_filename = os.path.basename(filename)
        target_path = os.path.join(TMP_DIR, safe_filename)
        
        if os.path.exists(target_path):
            try:
                os.remove(target_path)
                print(f"<p style='color:green'>✅ Deleted: {safe_filename}</p>")
                # Refresh file list after deletion
                files = [f for f in os.listdir(TMP_DIR) if os.path.isfile(os.path.join(TMP_DIR, f))]
            except Exception as e:
                print(f"<p style='color:red'>❌ Error: {str(e)}</p>")
        else:
            print("<p style='color:red'>❌ File not found.</p>")

# --- Handle GET (Display Form) ---
print("<h2>Delete Temporary File</h2>")
print('<form method="POST" action="/cgi-bin/deleteFile.py">')
print('<label for="file">Select file to delete:</label>')
print('<select name="file" id="file">')

if not files:
    print('<option value="">No files available</option>')
else:
    for f in files:
        # We only pass the filename to the value for security
        print(f'<option value="{f}">{f}</option>')

print('</select>')
print('<button type="submit">Delete File</button>')
print('</form>')
print('</div></body></html>')
