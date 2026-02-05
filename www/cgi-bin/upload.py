#!/usr/bin/env python3
import cgi
import os
import sys

# 1. Setup Directory
UPLOAD_DIR = "./www/uploads"
os.makedirs(UPLOAD_DIR, exist_ok=True)

# 2. Parse Form Data
form = cgi.FieldStorage()
fileitem = form['myfile'] if 'myfile' in form else None
# Check if the user clicked a specific file to view
view_file = form.getvalue('view') 

# 3. CSS Styling
css = """
<style>
    body { font-family: sans-serif; background-color: #0f172a; color: #f1f5f9; padding: 20px; display: flex; flex-direction: column; align-items: center; }
    .card { background-color: #1e293b; padding: 2rem; border-radius: 1rem; border: 1px solid rgba(255,255,255,0.1); box-shadow: 0 10px 25px rgba(0,0,0,0.3); width: 100%; max-width: 600px; }
    h1, h2 { color: #38bdf8; }
    .file-list { margin-top: 20px; border-top: 1px solid #475569; padding-top: 10px; }
    .file-item { display: flex; justify-content: space-between; align-items: center; padding: 8px; border-bottom: 1px solid #334155; }
    .file-item span { color: #94a3b8; }
    .preview { margin-top: 20px; text-align: center; border: 2px dashed #38bdf8; padding: 10px; border-radius: 10px; }
    img { max-width: 100%; border-radius: 5px; margin-top: 10px; }
    
    /* The button update */
    .btn { 
        background: #7dd3fc;  /* Light blue background */
        color: #0f172a;       /* Dark Navy/Black text color */
        padding: 5px 15px; 
        border-radius: 5px; 
        text-decoration: none; 
        font-weight: bold; 
        font-size: 0.9rem;
    }
    .btn:hover {
        background: #38bdf8;  /* Slightly darker blue on hover */
    }
</style>
"""

print("Content-Type: text/html\r\n\r\n")
print(f"<html><head>{css}</head><body><div class='card'>")

# --- PART A: Handle New Uploads ---
if fileitem is not None and getattr(fileitem, 'filename', None):
    filename = os.path.basename(fileitem.filename)
    with open(os.path.join(UPLOAD_DIR, filename), 'wb') as f:
        f.write(fileitem.file.read())
    print(f"<p style='color: #4ade80;'>Successfully uploaded: {filename}</p>")

# --- PART B: Display Selected File ---
if view_file:
    print(f"<div class='preview'><h2>Viewing: {view_file}</h2>")
    
    # Check if it's an image
    if view_file.lower().endswith(('.png', '.jpg', '.jpeg', '.gif', '.webp')):
        print(f"<img src='/uploads/{view_file}'>")
        # Added a spacer div for images too
        print("<div style='height: 20px;'></div>")
    else:
        # Added 'display: inline-block' and 'margin-bottom'
        print(f"""
            <p>No preview available for this file type.</p>
            <a href='/uploads/{view_file}' 
               class='btn' 
               style='background: #94a3b8; display: inline-block; margin-bottom: 20px;'>
               View / Download
            </a>
        """)
    print("</div>")

# --- PART C: List Files in Folder ---
print("<div class='file-list'><h2>Available Files</h2>")
files = os.listdir(UPLOAD_DIR)
if not files:
    print("<p>No files uploaded yet.</p>")
else:
    for f in files:
        # We create a link that reloads this same CGI script with ?view=filename
        print(f"<div class='file-item'><span>{f}</span> <a href='?view={f}' class='btn'>View</a></div>")
print("</div>")

print("<br><a href='javascript:history.back()' style='color: #94a3b8;'>&larr; Back to Upload</a>")
print("</div></body></html>")