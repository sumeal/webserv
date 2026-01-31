#!/usr/bin/python3
import sys, os, urllib.parse, tempfile

# Make sure temp folder exists
TEMP_DIR = "/tmp/cgi-data"
os.makedirs(TEMP_DIR, exist_ok=True)

# Only process POST requests
if os.environ.get("REQUEST_METHOD", "") != "POST":
    print("Content-Type: text/plain")
    print()
    print("This script only handles POST requests.")
    sys.exit(0)

# Get POST data
name = ""
try:
    cl = int(os.environ.get("CONTENT_LENGTH", 0))
    data = sys.stdin.read(cl)
    name = urllib.parse.parse_qs(data).get('name', [''])[0]
except:
    pass

# Analyze name
count = len(name.replace(' ', ''))
result_en, result_cn, result_pinyin, emoji = "UNKNOWN", "", "", ""
if count == 4:
    result_en="MACHO"; result_cn="猛男"; result_pinyin="MĚNGNÁN"; emoji="💪"
elif count == 3:
    result_en="COOL"; result_cn="酷"; result_pinyin="KÙ"; emoji="😎"
elif count == 20:
    result_en="SUPER HANDSOME GENTLEMAN"; result_cn="真的帅绅士"; result_pinyin="ZHĒN DE SHUÀI SHĒNSHÌ"; emoji="🤵✨"
elif count == 8:
    result_en="SO CUTE"; result_cn="太可爱了"; result_pinyin="TÀI KĚ'ÀI LE"; emoji="🙏💜"
elif count == 9:
    result_en="HANDSOME GENTLEMAN'S FRIEND"; result_cn="帅绅士的朋友"; result_pinyin="SHUÀI SHĒNSHÌ DE PÉNGYŎU"; emoji="🤵"
elif count < 8:
    result_en="MUKA AWAM"; result_cn="大众脸"; result_pinyin="DÀZHÒNGLIǍN"; emoji="😎"
else:
    result_en="GAYBOY"; result_cn="男同"; result_pinyin="NÁNTÓNG"; emoji="😄👏🌈"

# Save to temp file in TEMP_DIR
tmp_file = tempfile.NamedTemporaryFile(delete=False, dir=TEMP_DIR, mode='w', encoding='utf-8', prefix='nameanalysis_', suffix='.txt')
tmp_file.write(f"{name}\n{result_en}\n{result_cn}\n{result_pinyin}\n{emoji}\n{count}")
tmp_file.close()

# Send 303 redirect to showResult.py with filename (no full path exposed)
print("Status: 303 See Other")
print(f"Location: /cgi-bin/showResult.py?file={os.path.basename(tmp_file.name)}")
print("Content-Type: text/plain")
print()
