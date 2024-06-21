#include "HttpConnection.hpp"

void HttpConnection::sendBadRequestPage(SOCKET sockfd)
{
    // 403.htmlの内容を取得
    std::ifstream file("../documents/400.html");
    if (!file.is_open()) {
        perror("open error");
        // std::exit(EXIT_FAILURE);
    }
    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
        content += "\n";
    }
    file.close();

    std::string response;
    response = "HTTP/1.1 400 Bad Request\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;

    int status = send(sockfd, response.c_str(), response.length(), 0);
    if (status == 0){
        // delete events[sockfd];
        close(sockfd); //返り値が0のときは接続の失敗
    } //read/recv/write/sendが失敗したら返り値を0と-1で分けて処理する。その後クライアントをremoveする。
    if (status < 0)
    {
        perror("send error"); //返り値が-1のときはシステムコールの失敗
        // delete events[sockfd];
        close(sockfd);
        // std::exit(EXIT_FAILURE);
    }
}

void HttpConnection::sendForbiddenPage(SOCKET sockfd)
{
    // 403.htmlの内容を取得
    std::ifstream file("documents/403.html");
    if (!file.is_open()) {
        perror("open error");
        // std::exit(EXIT_FAILURE);
    }
    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
        content += "\n";
    }
    file.close();

    std::string response;
    response = "HTTP/1.1 403 Forbidden\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;

    sendToClient(sockfd, response);
}

void HttpConnection::sendNotAllowedPage(SOCKET sockfd)
{
    // 403.htmlの内容を取得
    std::ifstream file("documents/405.html");
    if (!file.is_open()) {
        perror("open error");
        // std::exit(EXIT_FAILURE);
    }
    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
        content += "\n";
    }
    file.close();

    std::string response;
    response = "HTTP/1.1 405 Method Not Allowed\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;

    sendToClient(sockfd, response);
}

void HttpConnection::requestEntityPage(SOCKET sockfd)
{
    // 403.htmlの内容を取得
    std::ifstream file("documents/413.html");
    if (!file.is_open()) {
        perror("open error");
        // std::exit(EXIT_FAILURE);
    }
    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
        content += "\n";
    }
    file.close();

    std::string response;
    response = "HTTP/1.1 413 Request Entity Too Large\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;

    sendToClient(sockfd, response);
}

void HttpConnection::sendNotImplementedPage(SOCKET sockfd)
{
    // 501.htmlの内容を取得
    std::ifstream file("documents/501.html");
    if (!file.is_open()) {
        perror("open error");
        // std::exit(EXIT_FAILURE);
    }
    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
        content += "\n";
    }
    file.close();

    std::string response;
    response = "HTTP/1.1 501 Not Implemented\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;

    sendToClient(sockfd, response);
}

void HttpConnection::sendTimeoutPage(SOCKET sockfd)
{
    // 504.htmlの内容を取得
    std::ifstream file("documents/504.html");
    if (!file.is_open()) {
        perror("open error");
        // std::exit(EXIT_FAILURE);
    }
    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
        content += "\n";
    }
    file.close();

    std::string response;
    response = "HTTP/1.1 504 Gateway Timeout\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;

    sendToClient(sockfd, response);
}

void HttpConnection::sendInternalErrorPage(SOCKET sockfd)
{
    // 504.htmlの内容を取得
    std::ifstream file("documents/500.html");
    if (!file.is_open()) {
        perror("open error");
        // std::exit(EXIT_FAILURE);
    }
    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
        content += "\n";
    }
    file.close();

    std::string response;
    response = "HTTP/1.1 500 Internal Server Error\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;

    sendToClient(sockfd, response);
}

// GETのcgi関数と違うのはPOSTメソッドで届いたクライアントからのリクエストボディをexecveの引数に渡すところ
void HttpConnection::executeCgi_postVersion(RequestParse& requestInfo, int pipe_c2p[2])
{
    close(pipe_c2p[R]);
    dup2(pipe_c2p[W],1);
    close(pipe_c2p[W]);
    extern char** environ;
    std::string cgiPath = "cgi_post/upload.cgi";
    std::string upload_dir = requestInfo.getPath();
    std::string request_body = requestInfo.getBody();
    char* const cgi_argv[] = { const_cast<char*>(cgiPath.c_str()), const_cast<char*>(upload_dir.c_str()), const_cast<char*>(request_body.c_str()), NULL };
    if(execve(cgi_argv[0], cgi_argv, environ) < 0)
        std::cout << "Error: execve() failed" << std::endl;
}

void HttpConnection::postProcess(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server)
{
    std::string directory = "upload";
    // 一応、/upload/ディレクトリがちゃんと存在するか確認
    struct stat info;
    if (stat(directory.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR))
    {
        sendForbiddenPage(sockfd);
        return ;
    }

    // もしclient_max_body_sizeが設定されていた場合
    if (server->serverSetting.find("client_max_body_size") != server->serverSetting.end())
    {
        std::string request_body = requestInfo.getBody();
        std::istringstream iss(server->serverSetting["client_max_body_size"]);
        unsigned long number;
        iss >> number;
        if (request_body.size() > number)
        {
            requestEntityPage(sockfd);
            return ;
        }
    }

    int pipe_c2p[2];
    if(pipe(pipe_c2p) < 0)
        throw std::runtime_error("Error: pipe() failed");
    pid_t pid = fork();
    if(pid == 0)
        executeCgi_postVersion(requestInfo, pipe_c2p); // GETのcgiと主に違うのはこの関数
    createResponseFromCgiOutput(pid, sockfd, pipe_c2p);
}

void HttpConnection::deleteProcess(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server)
{
    std::string file_path = requestInfo.getPath();
    if(file_path[0] == '/')
        file_path = file_path.substr(1);

    struct stat info;
    // 削除対象のファイル,ディレクトリの存在をチェック
    if (stat(file_path.c_str(), &info) != 0)
    {
        sendDefaultErrorPage(sockfd, server);//404エラー
        return ;
    }
    // 削除対象がディレクトリであるか確認
    if (S_ISDIR(info.st_mode))
    {
        sendForbiddenPage(sockfd);//403エラー
        return ;
    }
    // ファイルを削除
    if (std::remove(file_path.c_str()) != 0)
    {
        perror("remove error");
        // std::exit(EXIT_FAILURE);
    }

    std::string response;
    response = "HTTP/1.1 204 No Content\n";
    response += "Connection: Keep-Alive\n";
    // response += "Content-Length: 0\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Server: webserv/1.0.0\n";
    response += "\n";//ヘッダーとボディを分けるために、ボディが空でも必要

    sendToClient(sockfd, response);
}
