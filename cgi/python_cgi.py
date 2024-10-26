#!/usr/bin/python3
# -*- coding: utf-8 -*-
import os
import cgi
import codecs
import datetime

form = cgi.FieldStorage()

# 初回ロード時
if form.list == []:
    html = codecs.open('documents/view1.html', 'r', 'utf-8').read()
# SUBMITボタン押下時
else:
    html = codecs.open('documents/view2.html', 'r', 'utf-8').read()

# ステータス行を明示的に出力
print("HTTP/1.1 200 OK")
# Dateヘッダーを追加
now = datetime.datetime.utcnow()
print("Date:", now.strftime("%a, %d %b %Y %H:%M:%S GMT"))
# Content-Lengthを計算
content_length = len(html.encode('utf-8'))
print(f"Content-Length: {content_length}")
# Content-Typeを設定
print("Content-Type: text/html; charset=utf-8")
# Connectionヘッダーを追加（接続を閉じる場合）
print("Connection: keep-alive")

print("")
print(html + "\n")
