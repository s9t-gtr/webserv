#!/usr/bin/env python3
import os
import cgi

# クエリストリングの取得
query = os.environ.get("QUERY_STRING", "")
form = cgi.FieldStorage()

# HTMLレスポンスの作成
html = """\
Content-Type: text/html

<html>
<head><title>Query String Test</title></head>
<body>
    <h1>Received Query Parameters</h1>
    <ul>
"""
for key in form.keys():
    html += f"        <li><strong>{key}</strong>: {form.getvalue(key)}</li>\n"
html += """
    </ul>
    <p>Try adding <code>?name=John&age=25</code> to the URL!</p>
</body>
</html>
"""

# レスポンスを出力
print(html)
