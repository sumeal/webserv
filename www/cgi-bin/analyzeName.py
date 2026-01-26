#!/usr/bin/python3
# www/cgi-bin/analyzeName.py
import sys, os

# Get name from POST
name = ""
if 'CONTENT_LENGTH' in os.environ:
    cl = int(os.environ['CONTENT_LENGTH'])
    if cl > 0:
        data = sys.stdin.read(cl)
        # Find 'name=' in the string
        if 'name=' in data:
            # Get everything after 'name='
            name_part = data.split('name=')[1]
            # Stop at '&' if there are other parameters
            if '&' in name_part:
                name = name_part.split('&')[0]
            else:
                name = name_part
            # Decode URL encoding
            name = name.replace('+', ' ').replace('%20', ' ')

# Count letters
count = len(name.replace(' ', ''))

# Determine result
if count == 8:
    result = "CUTE. 可爱 😊"
elif count < 8:
    result = "MUKA AWAM. 大众脸 😎"
else:
    result = "GAYBOY. 男同😄"

# Output HTML with result
print("Content-Type: text/html")
print()
print(f"""
<!DOCTYPE html>
<html>
<body>
  <h1>Result for: {name}</h1>
  <h2 style="color: blue;">YOU ARE {result}!</h2>
  <p>Name length: {count} letters</p>
  <a href="/.html">Try another name</a>
</body>
</html>
""")