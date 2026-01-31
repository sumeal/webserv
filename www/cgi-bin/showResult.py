#!/usr/bin/python3
import sys, os

TEMP_DIR = "/tmp/cgi-data"

# Get file parameter from query string
query = os.environ.get("QUERY_STRING", "")
file_name = ""
if query.startswith("file="):
    file_name = query.split("file=")[1]

# Initialize default values
name, result_en, result_cn, result_pinyin, emoji, count = "", "", "", "", "", 0

# Read data from temp file
file_path = os.path.join(TEMP_DIR, file_name)
try:
    with open(file_path, "r", encoding="utf-8") as f:
        lines = [line.strip() for line in f.readlines()]
        if len(lines) == 6:
            name, result_en, result_cn, result_pinyin, emoji, count = lines
            count = int(count)
except:
    name = "Unknown"
    result_en = "ERROR"
    result_cn = "错误"
    result_pinyin = "-"
    emoji = "❌"
    count = 0

# Determine colors based on count
if count == 4: color="#9B59B6"; bg_color="linear-gradient(135deg, #E8DAEF 0%, #D7BDE2 100%)"
elif count == 3: color="#F39C12"; bg_color="linear-gradient(135deg, #FDEBD0 0%, #FAD7A0 100%)"
elif count == 20: color="#E74C3C"; bg_color="linear-gradient(135deg, #FADBD8 0%, #F5B7B1 100%)"
elif count == 8: color="#9B59B6"; bg_color="linear-gradient(135deg, #E8DAEF 0%, #D7BDE2 100%)"
elif count == 9: color="#8E44AD"; bg_color="linear-gradient(135deg, #EBDEF0 0%, #D2B4DE 100%)"
elif count < 8: color="#3498DB"; bg_color="linear-gradient(135deg, #D6EAF8 0%, #AED6F1 100%)"
else: color="#2ECC71"; bg_color="linear-gradient(135deg, #D5F4E6 0%, #A3E4D7 100%)"

# Output HTML
sys.stdout = open(sys.stdout.fileno(), mode='w', encoding='utf-8', buffering=1)
print("Content-Type: text/html; charset=utf-8")
print()
print(f"""
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<title>姓名分析结果 | Name Analysis Result</title>
<style>
body {{ font-family: 'Arial','Microsoft YaHei',sans-serif; background: linear-gradient(135deg,#667eea 0%,#764ba2 100%); margin:0; padding:20px; min-height:100vh; display:flex; justify-content:center; align-items:center; }}
.container {{ background:white; padding:40px; border-radius:25px; box-shadow:0 20px 40px rgba(0,0,0,0.2); max-width:600px; width:90%; text-align:center; }}
.result-box {{ background:{bg_color}; padding:30px; border-radius:20px; margin:30px 0; border:3px solid {color}; box-shadow:0 10px 20px rgba(0,0,0,0.1); }}
.result-main {{ font-size:48px; font-weight:bold; color:{color}; margin:0 0 15px 0; }}
.emoji {{ font-size:60px; margin:15px 0; }}
</style>
</head>
<body>
<div class="container">
<h1>姓名分析结果 | Name Analysis Result</h1>
<div class="result-box">
<div class="result-main">YOU ARE {result_en}!</div>
<div>{result_cn}</div>
<div>{result_pinyin}</div>
<div class="emoji">{emoji}</div>
<p>Name Entered: {name}</p>
<p>Character Count: {count}</p>
<a href="/">Back to Home</a>
</div>
</div>
</body>
</html>
""")
