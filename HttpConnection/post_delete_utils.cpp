#include "HttpConnection.hpp"


static std::string getStringFromHtml(std::string wantHtmlPath){
    std::ifstream file(wantHtmlPath);
    if (!file.is_open()) 
        perror("open error");
    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
        content += "\n";
    }
    file.close();
    return content;
}

void HttpConnection::sendBadRequestPage(SOCKET sockfd)
{
    std::string content = getStringFromHtml("documents/400.html");
    std::string response;
    response = "HTTP/1.1 400 Bad Request\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;

    sendToClient(sockfd, response);
}

void HttpConnection::sendForbiddenPage(SOCKET sockfd)
{
    std::string content = getStringFromHtml("documents/403.html");

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
    std::string content = getStringFromHtml("documents/405.html");

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
    std::string content = getStringFromHtml("documents/413.html");

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
    std::string content = getStringFromHtml("documents/501.html");

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

void HttpConnection::sendTimeoutPage(progressInfo *obj)
{
    obj->pHandler = NULL;
    // std::cerr << "debug: Status: Time out : socket: " << obj->socket << std::endl;
    kill(SIGKILL, obj->childPid);

    // 504.htmlの内容を取得
    std::string content = getStringFromHtml("documents/504.html");
    std::string response;
    response = "HTTP/1.1 504 Gateway Timeout\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + obj->httpConnection->getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;

    obj->httpConnection->sendToClient(obj->socket, response);
    obj->httpConnection->initProgressInfo(obj, obj->socket);
    createNewEvent(obj->socket, EVFILT_READ, EV_ADD, 0, 0, obj);
}

void HttpConnection::sendInternalErrorPage(SOCKET sockfd)
{
    std::string content = getStringFromHtml("documents/500.html");

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
    VirtualServer* server = requestInfo.getServer();
    std::string upload_dir = UPLOAD;
    std::string cgiPath = server->getCgiPath();
    std::string request_body = requestInfo.getBody();
    char* const cgi_argv[] = { const_cast<char*>(cgiPath.c_str()), const_cast<char*>(upload_dir.c_str()), const_cast<char*>(request_body.c_str()), NULL };
    if(execve(cgi_argv[0], cgi_argv, environ) < 0)
        exit(1);
}

void HttpConnection::postProcess(RequestParse& requestInfo, SOCKET sockfd, progressInfo *obj)
{
    std::string directory = UPLOAD;
    struct stat info;
    if (stat(directory.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR))
        return sendForbiddenPage(sockfd);
    VirtualServer* server = requestInfo.getServer();
    // もしclient_max_body_sizeが設定されていた場合
    if (server->serverSetting.find("client_max_body_size") != server->serverSetting.end())
    {
        std::string request_body = requestInfo.getBody();
        std::istringstream iss(server->serverSetting["client_max_body_size"]);
        unsigned long number;
        iss >> number;
        if (request_body.size() > number)
            return requestEntityPage(sockfd);
    }
    std::string cgiPath = server->getCgiPath();
    int pipe_c2p[2];
    if(pipe(pipe_c2p) < 0)
        throw std::runtime_error("Error: pipe() failed");
    obj->pipe_c2p[R] = pipe_c2p[R];
    obj->pipe_c2p[W] = pipe_c2p[W];
    obj->tHandler = sendTimeoutPage;
    obj->pHandler = confirmExitStatusFromCgi;
    pid_t pid = fork();
    if(pid == 0){
        executeCgi_postVersion(requestInfo, pipe_c2p); // GETのcgiと主に違うのはこの関数
    } else if(pid > 0) {
        obj->childPid = pid;
        createNewEvent(obj->socket, EVFILT_WRITE, EV_DELETE, 0, 0, obj);
        createNewEvent(obj->socket, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, 10000, obj);
        createNewEvent(pid, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, obj);
    } else {
        perror("fork() error");
    }
}

void HttpConnection::deleteProcess(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server)
{
    std::string file_path = requestInfo.getPath();
    struct stat info;
    if (stat(file_path.c_str(), &info) != 0)// 削除対象のファイル,ディレクトリの存在をチェック
        return sendDefaultErrorPage(sockfd, server);//404エラー
    if (S_ISDIR(info.st_mode))// 削除対象がディレクトリであるか確認
        return sendForbiddenPage(sockfd);//403エラー
    if (std::remove(file_path.c_str()) != 0)// ファイルを削除
        perror("remove error");

    std::string response;
    response = "HTTP/1.1 204 No Content\n";
    response += "Connection: Keep-Alive\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Server: webserv/1.0.0\n";
    response += "\n";//ヘッダーとボディを分けるために、ボディが空でも必要

    sendToClient(sockfd, response);
}
