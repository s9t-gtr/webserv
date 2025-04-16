#!/usr/bin/env python3
import os
import cgi

# クエリストリングの取得
form = cgi.FieldStorage()


# パラメータリストの生成
params_list = ""
for key in form.keys():
    params_list += f"        <li><strong>{key}</strong>: {form.getvalue(key)}</li>\r\n"

# レスポンスボディの定義
response_body = (
    "<!DOCTYPE html>\r\n"
    "<html lang='ja'>\r\n"
    "<head>\r\n"
    "    <meta charset='UTF-8'>\r\n"
    "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n"
    "    <title>Query String Test</title>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "    <h1>Received Query Parameters</h1>\r\n"
    "    <ul>\r\n"
    f"{params_list}"
    "    </ul>\r\n"
    "</body>\r\n"
    "</html>\r\n\r\n"
)

# レスポンスボディの出力
response = (
    "HTTP/1.1 200 OK\r\n"
    f"Content-Length: {len(response_body)}\r\n"
    f"Content-Type: text/html; charset=UTF-8\r\n"
    "\r\n"
    f"{response_body}"
)

print(response)