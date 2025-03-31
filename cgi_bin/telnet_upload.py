#!/usr/bin/env python3
import os
import sys
import json
import cgitb
from pathlib import Path

# デバッグ出力を有効化
cgitb.enable()

def write_debug_log(message):
    """デバッグログを書き込む"""
    with open('../upload/debug.log', 'a') as f:
        f.write(f"{message}\n")

def read_headers():
    """HTTPヘッダーを読み込む"""
    write_debug_log("Reading headers")
    headers = {}
    while True:
        line = sys.stdin.buffer.readline().decode('utf-8').strip()
        write_debug_log(f"Header line: '{line}'")
        if not line:  # 空行でヘッダー部終了
            break
        if ':' in line:
            key, value = line.split(':', 1)
            headers[key.strip().lower()] = value.strip()
    return headers

def read_chunked_data():
    """chunked encodingでデータを読み込む"""
    chunks = []
    write_debug_log("Starting to read chunks")
    
    try:
        headers = read_headers()
        write_debug_log(f"Headers: {headers}")
        
        if 'transfer-encoding' not in headers or 'chunked' not in headers['transfer-encoding'].lower():
            write_debug_log("Not a chunked transfer")
            return []

        while True:
            # チャンクサイズの行を読む
            size_line = sys.stdin.buffer.readline().decode('utf-8').strip()
            write_debug_log(f"Read size line: '{size_line}'")
            
            if not size_line:
                continue
                
            # 16進数のサイズを10進数に変換
            try:
                chunk_size = int(size_line, 16)
                write_debug_log(f"Chunk size: {chunk_size}")
            except ValueError as e:
                write_debug_log(f"Invalid chunk size: {size_line}, error: {e}")
                continue
                
            # チャンクサイズが0なら終了
            if chunk_size == 0:
                write_debug_log("Found end chunk (size 0)")
                break
                
            # チャンクデータを読む
            chunk = sys.stdin.buffer.read(chunk_size)
            chunks.append(chunk)
            write_debug_log(f"Read chunk: {chunk}")
            
            # チャンク後の改行を読み飛ばす
            sys.stdin.buffer.readline()
        
        write_debug_log(f"Total chunks read: {len(chunks)}")
        return chunks
        
    except Exception as e:
        write_debug_log(f"Error in read_chunked_data: {str(e)}")
        raise

def main():
    write_debug_log("\n=== Starting new request ===")
    
    # アップロードディレクトリの設定
    UPLOAD_DIR = Path("../upload")
    UPLOAD_DIR.mkdir(exist_ok=True)
    
    # レスポンスヘッダーを先に出力
    print("Content-Type: application/json")
    print()  # 空行が必要
    sys.stdout.flush()  # バッファをフラッシュ
    
    try:
        # chunkedデータの読み込み
        chunks = read_chunked_data()
        
        if not chunks:
            raise ValueError("No data received")
        
        # 1つ目のチャンクをファイル名として使用
        filename = chunks[0].decode('utf-8').strip()
        write_debug_log(f"Using filename: {filename}")
        
        # ファイルパスの作成
        file_path = UPLOAD_DIR / filename
        
        # 2つ目以降のチャンクをファイルに書き込み
        with open(file_path, 'wb') as f:
            for chunk in chunks[1:]:
                f.write(chunk)
        
        # 成功レスポンス
        response = {
            "success": True,
            "message": "File uploaded successfully",
            "filename": filename
        }
        
    except Exception as e:
        # エラーレスポンス
        response = {
            "success": False,
            "message": str(e)
        }
        write_debug_log(f"Error in main: {str(e)}")
    
    # JSONレスポンスの出力
    print(json.dumps(response))
    sys.stdout.flush()

if __name__ == "__main__":
    main()
