#!/usr/bin/python3
# www/cgi-bin/analyzeName.py
import sys, os
import urllib.parse

# Set UTF-8 encoding for output
sys.stdout = open(sys.stdout.fileno(), mode='w', encoding='utf-8', buffering=1)

print("Content-Type: text/html; charset=utf-8")
print()

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
            # Decode URL encoding properly
            name = urllib.parse.unquote(name.replace('+', ' '))

# Count letters (excluding spaces)
count = len(name.replace(' ', ''))

# Determine result with Pinyin
count = len(name.replace(' ', ''))

if count == 4:
    result_en = "MACHO"
    result_cn = "猛男"
    result_pinyin = "MĚNGNÁN"
    emoji = "💪"
    color = "#9B59B6"  # Purple
    bg_color = "linear-gradient(135deg, #E8DAEF 0%, #D7BDE2 100%)"
elif count == 3:
    result_en = "COOL"
    result_cn = "酷"
    result_pinyin = "KÙ"
    emoji = "😎"
    color = "#F39C12"  # Orange
    bg_color = "linear-gradient(135deg, #FDEBD0 0%, #FAD7A0 100%)"
elif count == 20:
    result_en = "SUPER HANDSOME GENTLEMAN"
    result_cn = "真的帅绅士"
    result_pinyin = "ZHĒN DE SHUÀI SHĒNSHÌ"
    emoji = "🤵✨"
    color = "#E74C3C"  # Red
    bg_color = "linear-gradient(135deg, #FADBD8 0%, #F5B7B1 100%)"
elif count == 8:
    result_en = "SO CUTE"
    result_cn = "太可爱了"
    result_pinyin = "TÀI KĚ'ÀI LE"
    emoji = "🙏💜"  # Folded hands + purple heart
    color = "#9B59B6"  # Purple
    bg_color = "linear-gradient(135deg, #E8DAEF 0%, #D7BDE2 100%)"  # Purple gradient
elif count == 9:
    result_en = "HANDSOME GENTLEMAN'S FRIEND"
    result_cn = "帅绅士的朋友"
    result_pinyin = "SHUÀI SHĒNSHÌ DE PÉNGYŎU"
    emoji = "🤵"
    color = "#8E44AD"  # Dark Purple
    bg_color = "linear-gradient(135deg, #EBDEF0 0%, #D2B4DE 100%)"
elif count < 8:
    result_en = "MUKA AWAM"
    result_cn = "大众脸"
    result_pinyin = "DÀZHÒNGLIǍN"
    emoji = "😎"
    color = "#3498DB"  # Blue
    bg_color = "linear-gradient(135deg, #D6EAF8 0%, #AED6F1 100%)"
else:
    result_en = "GAYBOY"
    result_cn = "男同"
    result_pinyin = "NÁNTÓNG"
    emoji = "😄👏🌈"
    color = "#2ECC71"  # Green
    bg_color = "linear-gradient(135deg, #D5F4E6 0%, #A3E4D7 100%)"

# HTML Output
print(f"""
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>姓名分析结果 | Name Analysis Result</title>
    <style>
        body {{
            font-family: 'Arial', 'Microsoft YaHei', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0;
            padding: 20px;
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
        }}
        
        .container {{
            background: white;
            padding: 40px;
            border-radius: 25px;
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.2);
            max-width: 600px;
            width: 90%;
            text-align: center;
        }}
        
        .title {{
            color: #2c3e50;
            font-size: 28px;
            margin-bottom: 30px;
            border-bottom: 3px solid #f0f0f0;
            padding-bottom: 15px;
        }}
        
        .result-box {{
            background: {bg_color};
            padding: 30px;
            border-radius: 20px;
            margin: 30px 0;
            border: 3px solid {color};
            box-shadow: 0 10px 20px rgba(0,0,0,0.1);
        }}
        
        .result-main {{
            font-size: 48px;
            font-weight: bold;
            color: {color};
            margin: 0 0 15px 0;
        }}
        
        .result-english {{
            font-size: 24px;
            color: #2c3e50;
            margin: 10px 0 5px 0;
            font-weight: bold;
        }}
        
        .result-chinese {{
            font-size: 32px;
            color: {color};
            margin: 10px 0 5px 0;
            font-weight: bold;
        }}
        
        .result-pinyin {{
            font-size: 18px;
            color: #7f8c8d;
            font-family: 'Courier New', monospace;
            margin: 5px 0 15px 0;
            letter-spacing: 1px;
        }}
        
        .emoji {{
            font-size: 60px;
            margin: 15px 0;
        }}
        
        .name-display {{
            background: #f8f9fa;
            padding: 20px;
            border-radius: 15px;
            margin: 20px 0;
            border-left: 5px solid {color};
        }}
        
        .name-label {{
            color: #7f8c8d;
            font-size: 16px;
            margin-bottom: 10px;
        }}
        
        .name-value {{
            font-size: 28px;
            color: #2c3e50;
            font-weight: bold;
            word-break: break-word;
        }}
        
        .stats {{
            display: flex;
            justify-content: space-around;
            margin: 30px 0;
            flex-wrap: wrap;
            gap: 20px;
        }}
        
        .stat-box {{
            background: #f0f7ff;
            padding: 20px;
            border-radius: 15px;
            flex: 1;
            min-width: 150px;
        }}
        
        .stat-label {{
            color: #7f8c8d;
            font-size: 14px;
            margin-bottom: 10px;
        }}
        
        .stat-value {{
            font-size: 36px;
            color: {color};
            font-weight: bold;
        }}
        
        .back-button {{
            background: linear-gradient(135deg, #4ecdc4 0%, #44a08d 100%);
            color: white;
            border: none;
            padding: 18px 40px;
            font-size: 18px;
            border-radius: 15px;
            cursor: pointer;
            transition: all 0.3s ease;
            font-weight: bold;
            box-shadow: 0 8px 20px rgba(78, 205, 196, 0.3);
            margin-top: 30px;
            text-decoration: none;
            display: inline-block;
        }}
        
        .back-button:hover {{
            transform: translateY(-3px);
            box-shadow: 0 12px 25px rgba(78, 205, 196, 0.4);
        }}
        
        .footer {{
            margin-top: 40px;
            color: #7f8c8d;
            font-size: 14px;
            border-top: 1px solid #eee;
            padding-top: 20px;
        }}
        
        @media (max-width: 600px) {{
            .container {{
                padding: 25px 20px;
            }}
            
            .result-main {{
                font-size: 36px;
            }}
            
            .result-chinese {{
                font-size: 28px;
            }}
            
            .stats {{
                flex-direction: column;
            }}
        }}
        
        @keyframes fadeIn {{
            from {{ opacity: 0; transform: translateY(20px); }}
            to {{ opacity: 1; transform: translateY(0); }}
        }}
        
        .container > * {{
            animation: fadeIn 0.5s ease forwards;
            opacity: 0;
        }}
        
        .container > *:nth-child(1) {{ animation-delay: 0.1s; }}
        .container > *:nth-child(2) {{ animation-delay: 0.2s; }}
        .container > *:nth-child(3) {{ animation-delay: 0.3s; }}
        .container > *:nth-child(4) {{ animation-delay: 0.4s; }}
        .container > *:nth-child(5) {{ animation-delay: 0.5s; }}
    </style>
</head>
<body>
    <div class="container">
        <h1 class="title">姓名分析结果 | Name Analysis Result</h1>
        
        <div class="name-display">
            <div class="name-label">您输入的姓名 | Name Entered:</div>
            <div class="name-value">{name}</div>
        </div>
        
        <div class="result-box">
            <div class="result-main">YOU ARE {result_en}!</div>
            <div class="result-chinese">{result_cn}</div>
            <div class="result-pinyin">{result_pinyin}</div>
            <div class="emoji">{emoji}</div>
        </div>
        
        <div class="stats">
            <div class="stat-box">
                <div class="stat-label">字母数量 | Character Count</div>
                <div class="stat-value">{count}</div>
            </div>
            <div class="stat-box">
                <div class="stat-label">空格 | Spaces</div>
                <div class="stat-value">{len(name) - count}</div>
            </div>
            <div class="stat-box">
                <div class="stat-label">总长度 | Total Length</div>
                <div class="stat-value">{len(name)}</div>
            </div>
        </div>
        
        <a href="/" class="back-button">
            返回主页 | Back to Home
        </a>
        
        <div class="footer">
            姓名分析工具 | XÌNGMÍNG FĒNXĪ GŌNGJÙ | Name Analysis Tool
        </div>
    </div>
</body>
</html>
""")