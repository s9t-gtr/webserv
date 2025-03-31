#!/usr/bin/env python3
import os

def generate_html():
    html = """
    <html>
    <head><title>Environment Variables</title></head>
    <body>
        <h1>Environment Variables</h1>
        <table border='1'>
            <tr><th>Variable</th><th>Value</th></tr>
    """
    for key, value in os.environ.items():
        html += f"<tr><td>{key}</td><td>{value}</td></tr>"
    
    html += """
        </table>
    </body>
    </html>
    """
    return html

def main():
    html_content = generate_html()
    response = f"""HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: {len(html_content)}\r\n\r\n{html_content}"""
    print(response)

if __name__ == "__main__":
    main()
