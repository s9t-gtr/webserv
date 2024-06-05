#include "HttpConnection.hpp"

// サーバーからのレスポンスヘッダーに含まれるGMT時刻を取得する関数
std::string HttpConnection::getGmtDate()
{
    std::time_t now = std::time(nullptr);
    std::tm* tm_now = std::gmtime(&now);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm_now);
    
    return (std::string(buffer));
}

// サーバーからのレスポンスとしてリダイレクトページを送る関数
void HttpConnection::sendRedirectPage(SOCKET sockfd)
{
    std::string response;
    response = "HTTP/1.1 301 Moved Permanently\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Length: 0\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Server: webserv/1.0.0\n";
    response += "Location: http://google.com\n";
    response += "\n";//ヘッダーとボディを分けるために、ボディが空でも必要

    if(send(sockfd, response.c_str(), response.length(), 0) < 0)
        std::cerr << "Error: send() failed" << std::endl;
    else
        std::cout << "send!!!!!!" << std::endl;
}

// サーバーからのレスポンスとしてデフォルトのエラーページを送る関数
void HttpConnection::sendDefaultErrorPage(SOCKET sockfd)
{
    // デフォルトのエラーページの内容を取得
    std::ifstream file("../documents/404.html");
    if (!file.is_open()) {
        perror("open error");
        std::exit(EXIT_FAILURE);
    }
    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
        content += "\n";
    }
    file.close();

    std::string response;
    response = "HTTP/1.1 404 Not Found\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;

    if(send(sockfd, response.c_str(), response.length(), 0) < 0)
        std::cerr << "Error: send() failed" << std::endl;
    else
        std::cout << "send!!!!!!" << std::endl;
}

// サーバーからのレスポンスとして静的ファイルを送る関数
void HttpConnection::sendStaticPage(RequestParse& requestInfo, SOCKET sockfd)
{
    std::string file_path = ".." + requestInfo.getPath();

    // もしパスが指定されていない、または、パスが/documents/ディレクトリのときは、デフォルトのindex.htmlページを送る
    if (requestInfo.getPath() == "/" || requestInfo.getPath() == "/documents/")
        file_path = "../documents/index.html";

    struct stat info;
    // 対象のファイル,ディレクトリの存在をチェックしつつ、infoに情報を読み込む
    if (stat(file_path.c_str(), &info) != 0)
    {
        sendDefaultErrorPage(sockfd);//404エラー
        return ;
    }
    // 削除対象がディレクトリであるか確認
    if (S_ISDIR(info.st_mode))
    {
        sendForbiddenPage(sockfd);//403エラー
        return ;
    }

    std::ifstream file(file_path);
    if (!file.is_open()) {
        sendDefaultErrorPage(sockfd);//404エラー <- std::exitのほうがいい？
        return ;
    }

    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
        content += "\n";
    }
    file.close();

    std::string response;
    response = "HTTP/1.1 200 OK\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;

    if(send(sockfd, response.c_str(), response.length(), 0) < 0)
        std::cerr << "Error: send() failed" << std::endl;
    else
        std::cout << "send!!!!!!" << std::endl;
}
