#include <iostream>
#include <string>
#include <sstream>

int main(void){
  std::string responseBody = "<html><body><h1>This page made by CGI!</h1></body></html>";
  std::string::size_type contentLength = responseBody.length();
  std::stringstream ss;
  ss << contentLength;
  std::string contentLengthStr = ss.str();

  std::string strOutData;
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
  strOutData += "<html><body><h1>This page made by CGI!</h1></body></html>\n\n";

  std::cout << strOutData.c_str();
}