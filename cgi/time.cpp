#include <iostream>
#include <ctime>
#include <string>
#include <sstream>
using namespace std;

std::string getCurrentTime() {
    // 現在時刻を取得
    std::time_t currentTime = std::time(0);
    
    // 現在時刻をローカル時間に変換
    std::tm* localTime = std::localtime(&currentTime);
    
    // 時刻を整形してstd::stringに格納
    std::ostringstream oss;
    oss << (localTime->tm_year + 1900) << "-"
        << (localTime->tm_mon + 1) << "-"
        << localTime->tm_mday << " "
        << localTime->tm_hour << ":"
        << localTime->tm_min << ":"
        << localTime->tm_sec;
    
    return oss.str();
}

int main(void)
{
  /* 出力データを文字列としてためておくための変数 */
  string strOutData;
  std::string now = getCurrentTime();

  std::string responseBody = "<html><body><h1>" + now + "</h1></body></html>";
  std::string::size_type contentLength = responseBody.length();
  std::stringstream ss;
  ss << contentLength;
  std::string contentLengthStr = ss.str();

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
  strOutData += "Content-Length: " + contentLengthStr + "\n";
  strOutData += "Keep-Alive: timeout=5, max=100\n";
  strOutData += "Connection: Keep-Alive\n";
  strOutData += "Content-Type: text/html\n";
  strOutData += "\n";
  strOutData += responseBody;
  /* 標準出力に設定内容を出力 */
  std::cout << strOutData.c_str();
}
