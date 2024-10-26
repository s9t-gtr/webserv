#include <iostream>
#include <string>

int main(void){
  std::string strOutData;

  strOutData = "HTTP/1.1 200 OK\n";
  strOutData += "Date: Wed, 28 Oct 2020 07:57:45 GMT\n";
  strOutData += "Server: nginx\n";
  strOutData += "Content-Location: index.html.en\n";
  strOutData += "Vary: negotiate\n";
  strOutData += "TCN: choice\n";
  strOutData += "Last-Modified: Thu, 29 Aug 2019 05:05:59 GMT\n";
  strOutData += "Accept-Ranges: bytes\n";
  strOutData += "Content-Length: 57\n";
  strOutData += "Keep-Alive: timeout=5, max=100\n";
  strOutData += "Connection: Keep-Alive\n";
  strOutData += "Content-Type: text/html\n";
  strOutData += "\n";
  strOutData += "<html><body><h1>This page made by CGI!</h1></body></html>";

  std::cout << strOutData.c_str();
}