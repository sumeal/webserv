#!/usr/bin/env python3
import cgi
import os
import sys

# 1. Setup Directory
# Note: This is relative to where you run ./anonymous.exe
UPLOAD_DIR = "./www/uploads"
os.makedirs(UPLOAD_DIR, exist_ok=True)

# 2. Parse Form Data
form = cgi.FieldStorage()
fileitem = form['myfile'] if 'myfile' in form else None

# 3. Prepare Response Body with Dark Mode CSS
# Using the same color palette as your HTML
css = """
<style>
    body { 
        font-family: sans-serif; 
        background-color: #0f172a; 
        color: #f1f5f9; 
        display: flex; 
        justify-content: center; 
        padding-top: 50px;
    }
    .card {
        background-color: #1e293b;
        padding: 2rem;
        border-radius: 1rem;
        border: 1px solid rgba(255,255,255,0.1);
        box-shadow: 0 10px 25px rgba(0,0,0,0.3);
        max-width: 500px;
        width: 100%;
    }
    h1 { color: #38bdf8; margin-top: 0; }
    code { background: #0f172a; padding: 2px 5px; border-radius: 4px; color: #7dd3fc; }
    .debug { color: #94a3b8; font-size: 0.8rem; margin-top: 2rem; border-top: 1px solid #475569; pt: 1rem; }
    a { color: #38bdf8; text-decoration: none; border: 1px solid #38bdf8; padding: 5px 10px; border-radius: 5px; display: inline-block; margin-top: 10px; }
    a:hover { background: #38bdf8; color: #0f172a; }
</style>
"""

response_html = f"<html><head><title>Upload Result</title>{css}</head><body><div class='card'>"

# The fix: explicitly check "is not None" to avoid the __bool__ TypeError
if fileitem is not None and getattr(fileitem, 'filename', None):
    filename = os.path.basename(fileitem.filename)
    filepath = os.path.join(UPLOAD_DIR, filename)

    try:
        with open(filepath, 'wb') as f:
            f.write(fileitem.file.read())
        
        response_html += f"<h1>Success!</h1>"
        response_html += f"<p>File <code>{filename}</code> has been uploaded.</p>"
        response_html += f"<p>Path: <code>{os.path.abspath(filepath)}</code></p>"
    except Exception as e:
        response_html += f"<h1>Upload Error</h1>"
        response_html += f"<p style='color: #ef4444;'>{str(e)}</p>"
else:
    response_html += "<h1>Missing File</h1>"
    response_html += "<p>No file was selected or the field name was incorrect.</p>"

response_html += '<a href="javascript:history.back()">&larr; Go Back</a>'

# Debugging info
response_html += "<div class='debug'><h3>CGI Debug Info:</h3>"
response_html += f"<p>Form Keys: {list(form.keys())}</p></div>"
response_html += "</div></body></html>"

# 4. Output Headers and Body
print("Content-Type: text/html")
print() # The mandatory blank line
print(response_html)