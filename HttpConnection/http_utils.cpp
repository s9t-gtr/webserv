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

    int status = send(sockfd, response.c_str(), response.length(), 0);
    if (status == 0){
        delete events[sockfd];
        close(sockfd); //返り値が0のときは接続の失敗
    } //read/recv/write/sendが失敗したら返り値を0と-1で分けて処理する。その後クライアントをremoveする。
    if (status < 0)
    {
        perror("send error"); //返り値が-1のときはシステムコールの失敗
        delete events[sockfd];
        close(sockfd);
        std::exit(EXIT_FAILURE);
    }
}

// サーバーからのレスポンスとしてデフォルトのエラーページを送る関数
void HttpConnection::sendDefaultErrorPage(SOCKET sockfd, VirtualServer* server)
{
    std::string error_page_path = server->serverSetting["error_page"];
    // デフォルトのエラーページの内容を取得
    std::ifstream file(error_page_path);
    if (!file.is_open()) {
        //ファイルパスが無効ならデフォルトを設定
        error_page_path = "../documents/404_default.html";
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

    int status = send(sockfd, response.c_str(), response.length(), 0);
    if (status == 0){
        delete events[sockfd];
        close(sockfd); //返り値が0のときは接続の失敗
    } //read/recv/write/sendが失敗したら返り値を0と-1で分けて処理する。その後クライアントをremoveする。
    if (status < 0)
    {
        perror("send error"); //返り値が-1のときはシステムコールの失敗
        delete events[sockfd];
        close(sockfd);
        std::exit(EXIT_FAILURE);
    }
}

// サーバーからのレスポンスとして静的ファイルを送る関数
void HttpConnection::sendStaticPage(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server, Location* location)
{
    std::string file_path = location->locationSetting["root"] + requestInfo.getPath();
    // std::cout << "========" << file_path << "========" << std::endl; //デバッグ
    // もしパスが指定されていないときは、デフォルトのindex.htmlページを送る
    if (requestInfo.getPath() == "/")
        file_path = "../documents/index.html";

    struct stat info;
    // 対象のファイル,ディレクトリの存在をチェックしつつ、infoに情報を読み込む
    if (stat(file_path.c_str(), &info) != 0)
    {
        sendDefaultErrorPage(sockfd, server);//404エラー
        return ;
    }
    // 対象がディレクトリであるか確認
    if (S_ISDIR(info.st_mode))
    {
        // indexが設定されていたらそのファイルを出す
        if (location->locationSetting["index"] != "none")
            file_path = location->locationSetting["index"];
        else if (location->locationSetting["autoindex"] == "on")
        {
            sendAutoindexPage(requestInfo, sockfd, server, location);
            return ;
        }
        else
        {
            sendForbiddenPage(sockfd);//403エラー
            return ;
        }
    }

    std::ifstream file(file_path);
    if (!file.is_open()) {
        sendDefaultErrorPage(sockfd, server);//404エラー <- std::exitのほうがいい？
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
    int status = send(sockfd, response.c_str(), response.length(), 0);
    if (status == 0){
        delete events[sockfd];
        close(sockfd); //返り値が0のときは接続の失敗
    } //read/recv/write/sendが失敗したら返り値を0と-1で分けて処理する。その後クライアントをremoveする。
    if (status < 0)
    {
        perror("send error"); //返り値が-1のときはシステムコールの失敗
        delete events[sockfd];
        close(sockfd);
    }
}
