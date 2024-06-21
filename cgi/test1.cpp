/* C++ */
#include <iostream>
#include <string>
using namespace std;

int main(void)
{
  /* 出力データを文字列としてためておくための変数 */
  string strOutData;

  // while (1)
  // {
  //   int a;
  //   (void)a;
  // }

  /* httpヘッダ */
  /* HTML出力であることを出力 */
strOutData = "HTTP/1.1 200 OK\n";
strOutData += "Date: Wed, 28 Oct 2020 07:57:45 GMT\n";
strOutData += "Server: Apache/2.4.41 (Unix)\n";
strOutData += "Content-Location: index.html.en\n";
strOutData += "Vary: negotiate\n";
strOutData += "TCN: choice\n";
strOutData += "Last-Modified: Thu, 29 Aug 2019 05:05:59 GMT\n";
strOutData += "ETag: \"";
strOutData += "2d-5913a76187bc0\"";
strOutData += "\n";
strOutData += "Accept-Ranges: bytes\n";
strOutData += "Content-Length: 44\n";
strOutData += "Keep-Alive: timeout=5, max=100\n";
strOutData += "Connection: Keep-Alive\n";
strOutData += "Content-Type: text/html\n";
strOutData += "\n";
strOutData += "<html><body><h1>This page made by CGI!</h1></body></html>";
  /* 標準出力に設定内容を出力 */
  std::cout << strOutData.c_str();
}