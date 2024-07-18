#include "HttpConnection.hpp"

void HttpConnection::sendToClient(SOCKET sockfd, std::string response){
    // std::cerr << DEBUG << "sendtoClient()" << std::endl;

    int status = send(sockfd, response.c_str(), response.length(), 0);
    if (status == 0){
        // delete obj;
        perror("send error: client connection close");
        printf("errno = %d (%s)\n", errno, strerror(errno));
        close(sockfd);
    } 
    else if (status < 0){
        // delete obj;
        perror("send error: failed systemcall"); 
        printf("errno = %d (%s)\n", errno, strerror(errno));
        close(sockfd);
    }

}

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
void HttpConnection::sendRedirectPage(SOCKET sockfd, Location* location)
{
    std::string response;
    response = "HTTP/1.1 301 Moved Permanently\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Length: 0\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Server: webserv/1.0.0\n"; 
    response += "Location: " + location->locationSetting["return"] + "\n";
    response += "\n";//ヘッダーとボディを分けるために、ボディが空でも必要

    sendToClient(sockfd, response);

}

// サーバーからのレスポンスとしてデフォルトのエラーページを送る関数
void HttpConnection::sendDefaultErrorPage(SOCKET sockfd, VirtualServer* server)
{
    std::string error_page_path = server->serverSetting["error_page"];
    // デフォルトのエラーページの内容を取得
    std::ifstream file(error_page_path);
    if (!file.is_open()) {
        //ファイルパスが無効ならデフォルトを設定
        error_page_path = "documents/404_default.html";
        file.close();  // 既存のファイルストリームを閉じる
        file.open(error_page_path);  // 新しいファイルを開く
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

    sendToClient(sockfd, response);
}

// サーバーからのレスポンスとして静的ファイルを送る関数
void HttpConnection::sendStaticPage(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server, Location* location)
{
    std::string file_path = requestInfo.getPath();
    struct stat info;
    // 対象のファイル,ディレクトリの存在をチェックしつつ、infoに情報を読み込む
    if (stat(file_path.c_str(), &info) != 0)
        return sendDefaultErrorPage(sockfd, server);//404エラー
    // 対象がディレクトリであるか確認
    if (S_ISDIR(info.st_mode))
    {
        // indexが設定されていたらそのファイルを出す
        if (location->locationSetting["index"] != "none")
            file_path = location->locationSetting["index"];
        else if (location->locationSetting["autoindex"] == "on")
            return sendAutoindexPage(requestInfo, sockfd, server, location);
        else
            return sendForbiddenPage(sockfd);//403エラー
    }

    std::ifstream file(file_path);
    if (!file.is_open())
        return sendDefaultErrorPage(sockfd, server);//404エラー <- std::exitのほうがいい？
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
    sendToClient(sockfd, response);
}

bool HttpConnection::checkCompleteRecieved(progressInfo obj){
    std::string::size_type head = 0;
    std::string::size_type tail = obj.buffer.find("\n");
    while(obj.content_length == 0 && tail != std::string::npos){
        std::string line = obj.buffer.substr(head, tail);
        std::string::size_type target = line.find("Content_Length:");
        if(target != std::string::npos){
            std::stringstream ss;
            ss << line.substr(target+15);
            ss >> obj.content_length;
        }
        head = tail + 1;
        tail = obj.buffer.find("\n", head);
    }
    if(isReadNewLine(obj.buffer) && obj.content_length == 0){
        // std::cerr << DEBUG << "Complete recieved" << std::endl;
        return true; //get method && no body request
    }
    else if(isReadNewLine(obj.buffer) && bodyConfirm(obj)){
        // std::cerr << DEBUG << "Complete recieved" << std::endl;
        return true; // already read body of request
    }
    return false;
}

bool HttpConnection::isReadNewLine(std::string buffer){
    if(buffer.find("\n\r\n") != std::string::npos || buffer.find("\n\n") != std::string::npos)
        return true;
    return false;
}

bool HttpConnection::bodyConfirm(progressInfo info){
    size_t returnLen = std::strlen("\n\r\n");
    std::string::size_type newLineIdx = info.buffer.find("\r\n");
    if(newLineIdx == std::string::npos){
        returnLen = std::strlen("\n");
        newLineIdx = info.buffer.find("\n\n");
    }
    std::string tmpBody = info.buffer.substr(newLineIdx+returnLen);
    if(tmpBody.size() == info.content_length)
        return true;
    return false;
}