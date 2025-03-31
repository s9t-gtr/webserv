#!/usr/bin/env python3
import os
import sys
import cgi
import cgitb
cgitb.enable()

def main():
    # リクエストメソッドの確認
    request_method = os.environ.get('REQUEST_METHOD', '')
    path_info = os.environ.get('PATH_INFO', '')

    # DELETEメソッド以外は405を返す
    if request_method != 'DELETE':
        print("Status: 405 Method Not Allowed")
        print("Content-Type: text/plain")
        print()
        print("Method Not Allowed")
        return

    # パスの組み立て
    # /dummy.html のような形式で来るので、先頭の / を削除
    if path_info.startswith('/'):
        path_info = path_info[1:]

    file_path = os.path.join('/usr/share/nginx/documents', path_info)

    # パスのバリデーション
    if not os.path.exists(file_path):
        print("Status: 404 Not Found")
        print("Content-Type: text/plain")
        print()
        print("File not found")
        return

    # ファイル削除の実行
    try:
        os.remove(file_path)
        print("Status: 204 No Content")
        print()
    except Exception as e:
        print("Status: 500 Internal Server Error")
        print("Content-Type: text/plain")
        print()
        print(f"Error: {str(e)}")

if __name__ == "__main__":
    main()
