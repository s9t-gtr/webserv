#!/usr/bin/python3

import os
import sys
#log_file = "./cgi_debug.log"
#with open(log_file, "a") as log:
#    log.write("=== CGI Debug Start ===\n")
#
#    def log_message(message):
#        log.write(message + "\n")
#        log.flush()  # すぐに書き込む
#
#    log_message(f"CONTENT_LENGTH={os.environ.get('CONTENT_LENGTH', 'None')}")
#
#    def get_request_body():
#        content_length = os.environ.get("CONTENT_LENGTH")
#        if content_length is None:
#            log_message("ERROR: CONTENT_LENGTH is not set")
#            return ""
#        
#        try:
#            content_length = int(content_length)
#            if content_length > 0:
#                body = sys.stdin.read(content_length)
#                log_message(f"Received Body: {body}")
#                return body
#        except ValueError:
#            log_message(f"ERROR: Invalid CONTENT_LENGTH={content_length}")
#        
#        return ""
#    request_body = get_request_body()
#    log_message("=== CGI Debug End ===")
def get_request_body():
    """標準入力からリクエストボディを取得"""
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    if content_length > 0:
        return sys.stdin.read(content_length)
    return ""
# リクエストボディの取得

request_body = get_request_body()
print(f"DEBUG: CONTENT_LENGTH={os.environ.get('CONTENT_LENGTH', 'None')}", file=sys.stderr)
# HTTPレスポンスの出力
print("Content-Type: text/html; charset=utf-8")
print("")  # 空行はヘッダーとボディの区切り
print(f"""<!DOCTYPE html>
<html lang="ja">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CGI Response</title>
    <style>
        body {{ font-family: Arial, sans-serif; padding: 20px; }}
        h1 {{ color: #333; }}
        pre {{ background: #f4f4f4; padding: 10px; border-radius: 5px; }}
    </style>
</head>
<body>
    <h1>Received Request Body</h1>
    <pre>{request_body}</pre>
</body>
</html>""")

