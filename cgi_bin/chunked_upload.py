#!/usr/bin/env python3
import os
import sys
import cgi
import cgitb
import json
from pathlib import Path

# デバッグ用にエラーを表示
cgitb.enable()

def log(message):
    """デバッグ用ログ出力"""
    sys.stderr.write(f"{message}\n")
    sys.stderr.flush()

def main():
    log("CGI script started")

    # アップロードディレクトリの設定
    UPLOAD_DIR = Path("../upload")
    UPLOAD_DIR.mkdir(exist_ok=True)

    # ヘッダー設定
    print("Content-Type: application/json")
    print()

    try:
        # 環境変数から `CONTENT_LENGTH` を取得
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))

        if content_length > 0:
            data = sys.stdin.read(content_length)  # ここで `stdin` からデータを読む
            log("Received data (first 100 chars): {data[:100]}")  # 最初の100文字を表示
        else:
            log("No data received!")

        print("Done reading stdin")
        log("Reading form data...")
        # フォームデータの取得
        form = cgi.FieldStorage()
        log("Form data read complete")

        # 必要なパラメータの取得
        chunk = form['chunk']
        chunk_index = int(form.getvalue('chunkIndex', "-1"))
        total_chunks = int(form.getvalue('totalChunks', "-1"))
        filename = form.getvalue('fileName', "unknown")

        log(f"chunkIndex: {chunk_index}, totalChunks: {total_chunks}, filename: {filename}")

        if chunk_index == -1 or total_chunks == -1:
            raise ValueError("Missing required parameters")

        # 一時ファイル名の生成
        temp_filename = f"{filename}.part{chunk_index}"
        final_path = UPLOAD_DIR / filename
        temp_path = UPLOAD_DIR / temp_filename

        # チャンクの保存
        if chunk.file:
            log(f"Saving chunk {chunk_index} to {temp_path}")
            with open(temp_path, 'wb') as f:
                f.write(chunk.file.read())
            log(f"Chunk {chunk_index} saved successfully")

        # 全チャンクが揃ったかチェック
        existing_chunks = len(list(UPLOAD_DIR.glob(f"{filename}.part*")))
        log(f"Existing chunks: {existing_chunks}/{total_chunks}")

        if existing_chunks == total_chunks:
            log("All chunks received. Merging files...")
            with open(final_path, 'wb') as outfile:
                for i in range(total_chunks):
                    part_path = UPLOAD_DIR / f"{filename}.part{i}"
                    with open(part_path, 'rb') as infile:
                        outfile.write(infile.read())
                    # 一時ファイルの削除
                    os.remove(part_path)

            response = {
                "success": True,
                "message": "Upload completed",
                "filename": filename
            }
        else:
            response = {
                "success": True,
                "message": "Chunk received",
                "chunksReceived": existing_chunks,
                "totalChunks": total_chunks
            }

    except Exception as e:
        log(f"Error: {e}")
        response = {
            "success": False,
            "message": str(e)
        }

    # JSONレスポンスの送信
    print(json.dumps(response))
    log("Response sent successfully")

if __name__ == "__main__":
    main()
