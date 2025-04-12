#include "HttpConnection.hpp"

void HttpConnection::sendToClient(std::string response, progressInfo *obj, int kind){
    /*
        kind: 
            normal: 0 - 通信は切断しないしwriteイベントもまだ削除していない
            cgi: 1 - cgi実行時にWRITEイベントは削除するため分岐が必要
            bad_request: 2 - objをdeleteしつつsocketもcloseするので分岐が必要
    
    */
    // std::cerr << DEBUG << "sendtoClient()" << std::endl;
    // std::cerr << DEBUG << GRAY << "======================= response =======================" << std::endl;
    // std::cerr << DEBUG << obj->buffer << RESET << std::endl;
    if((size_t)obj->sndbuf < response.length()){
        obj->buffer = response.substr(obj->sndbuf);
        obj->tmpKind = kind;
        response = response.substr(0, obj->sndbuf);
        obj->wHandler = sendLargeResponse;
    }else
        obj->buffer = "";
    int status = send(obj->socket, response.c_str(), response.length(), 0);
    if (status == 0){
        perror("send error: client connection close");
        kind = 2;
    } 
    else if (status < 0){
        if(kind == SEND)//send()の失敗による kind=SENDときのsendInternalErrorPage()送信時のsend()でも失敗したら
            kind = BAD_REQ;
        else
            sendInternalErrorPage(obj, SEND); 
    }
    if(obj->buffer == ""){//largeResponse送信途中にobjをリセットしたくない
        if(kind == FORK){
            close(obj->pipe_c2p[R]);
            close(obj->pipe_c2p[W]);
        }
        if(kind == BAD_REQ){//bad_requestする時にはsocketとobjを消すのでinitもreadイベント生成も不要
            // std::cerr << DEBUG << "---- close: socket ----: " << obj->socket << std::endl;
            close(obj->socket);
            delete obj;
            obj = NULL;
            return ;
        }
        if(kind != RECV){ //recvの時はinitだけすれば良い
            if(kind != CGI_FAIL){ //writeをaddする前にsendInternalErrirに入るのでDELETEしない
                createNewEvent(obj->socket, EVFILT_WRITE, EV_DELETE, 0, 0, obj);
            }
            createNewEvent(obj->socket, EVFILT_READ, EV_ADD, 0, 0, obj);
        }
        if(kind == CLOSE){
            close(obj->socket);
            delete obj;
        }
        obj->httpConnection->initProgressInfo(obj, obj->socket, obj->sndbuf);
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
void HttpConnection::sendRedirectPage(Location* location, progressInfo *obj)
{
    std::string response;
    response = "HTTP/1.1 301 Moved Permanently\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Length: 0\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Server: webserv/1.0.0\n"; 
    response += "Location: " + location->locationSetting["return"] + "\n";
    response += "\n";//ヘッダーとボディを分けるために、ボディが空でも必要

    sendToClient(response, obj, NORMAL);

}

// サーバーからのレスポンスとしてデフォルトのエラーページを送る関数
void HttpConnection::sendDefaultErrorPage(VirtualServer* server, progressInfo *obj)
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

    sendToClient(response, obj, NORMAL);
}

// サーバーからのレスポンスとして静的ファイルを送る関数
void HttpConnection::sendStaticPage(RequestParse& requestInfo, progressInfo *obj)
{
    std::string file_path = requestInfo.getPath();
    Location *location = requestInfo.getLocation();
    VirtualServer *server = requestInfo.getServer();
    struct stat info;
    // 対象のファイル,ディレクトリの存在をチェックしつつ、infoに情報を読み込む
    if (stat(file_path.c_str(), &info) != 0)
        return sendDefaultErrorPage(requestInfo.getServer(),  obj);//404エラー
    // 対象がディレクトリであるか確認
    if (S_ISDIR(info.st_mode))
    {
        // indexが設定されていたらそのファイルを出す
        if (location->locationSetting["index"] != "none")
            file_path = location->locationSetting["index"];
        else if (location->locationSetting["autoindex"] == "on")
            return sendAutoindexPage(requestInfo, obj);
        else
            return sendForbiddenPage(obj);//403エラー
    }
    if(file_path == "documents/user.html" || file_path == "documents/login.html"){
        return cookiePage(requestInfo, obj);
    }
        
    std::ifstream file(file_path);
    if (!file.is_open())
        return sendDefaultErrorPage(server, obj);//404エラー <- std::exitのほうがいい？
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
    if(requestInfo.getHeader("Connection") == "close"){
        return sendToClient(response, obj, CLOSE);
    }
    sendToClient(response, obj, NORMAL);
}

bool HttpConnection::checkCompleteRecieved(progressInfo obj){
    std::string::size_type head = 0;
    std::string::size_type tail = obj.buffer.find("\n");
    while(obj.content_length == 0 && tail != std::string::npos){
        std::string line = obj.buffer.substr(head, tail);
        std::string::size_type target = line.find("Content-Length:");
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
    size_t returnLen = std::strlen("\r\n\r\n");
    std::string::size_type newLineIdx = info.buffer.find("\r\n\r\n");
    if(newLineIdx == std::string::npos){
        returnLen = std::strlen("\n\n");
        newLineIdx = info.buffer.find("\n\n");
    }
    std::string tmpBody = info.buffer.substr(newLineIdx+returnLen);
    if(tmpBody.size() == info.content_length)
        return true;
    return false;
}

void HttpConnection::cookiePage(RequestParse& requestInfo, progressInfo *obj){
    std::string cookieInfo = requestInfo.getHeader("Cookie");
    if(requestInfo.getPath() == "documents/user.html"){
        if(cookieInfo != "" && requestInfo.searchSessionId(cookieInfo)){
            std::vector<std::string> vec =  requestInfo.getUserInfo(cookieInfo);
            sendUserPage(vec, 200, requestInfo, obj);//cgiでhtmlを成形し、responseヘッダも繋げたものを返す
        }else{
            sendLoginPage(303, requestInfo, obj);
        }
    }else if (requestInfo.getPath() == "documents/login.html"){
        if(cookieInfo != "" && requestInfo.searchSessionId(cookieInfo)){
            std::vector<std::string> vec =  requestInfo.getUserInfo(cookieInfo);
            sendUserPage(vec, 303, requestInfo, obj);//cgiでhtmlを成形し、responseヘッダも繋げたものを返す
        }else{
            sendLoginPage(200, requestInfo, obj);
        }
    }
}
std::string HttpConnection::createRedirectPath(std::string requestPath){
    size_t idx = requestPath.rfind("/");
    std::string path = requestPath.substr(0, idx+1);
    if(requestPath.find("user.html") != std::string::npos){
        path += "login.html";
    }else if(requestPath.find("login.html") != std::string::npos){
        path += "user.html";
    }else{
        // std::cerr << DEBUG << "storange: createRedirectPath()" << std::endl;
    }
    return path;
}

void HttpConnection::sendLoginPage(int status, RequestParse& requestInfo, progressInfo *obj){
    if(status == 303){
        std::string redirectPath = createRedirectPath(requestInfo.getRawPath());
        std::string response = "HTTP/1.1 303 See Other\n";
        response += "Connection: Keep-Alive\n";
        response += "Content-Length: "+ std::to_string(std::strlen("Login successful")) + "\n";
        response += "Date: " + getGmtDate() + "\n";
        response += "Server: webserv/1.0.0\n";
        response += "Location: " + redirectPath + "\n";
        response += "\n";
        response += "Login successful";
        sendToClient(response, obj, NORMAL);
        return ;
    }
    std::string content = getStringFromHtml("documents/login.html");
    std::string response;
    response = "HTTP/1.1 200 OK\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;
    sendToClient(response, obj, NORMAL);
}

void HttpConnection::sendUserPage(std::vector<std::string> userInfo, int status, RequestParse& requestInfo, progressInfo *obj){
    /*
        htmlファイルをuser情報に書き換え、responseヘッダも連結した文字列を出力するCGI
    */
   if(status == 303){
        std::string redirectPath = createRedirectPath(requestInfo.getRawPath());
        std::string response = "HTTP/1.1 303 See Other\n";
        response += "Connection: Keep-Alive\n";
        response += "Content-Length: "+ std::to_string(std::strlen("Login successful")) + "\n";
        response += "Date: " + getGmtDate() + "\n";
        response += "Server: webserv/1.0.0\n";
        response += "Location: " + redirectPath + "\n";
        response += "\n";
        response += "Login successful";

        sendToClient(response, obj, NORMAL);
        return ;
   }
   std::ifstream ifs("documents/user.html");
   if(!ifs){
        perror("sendUserPage: documents/user.html");
   }
   std::string line;
   std::string content;
    while(!(ifs).eof()){
        std::getline(ifs, line);
        if(line.find("Welcome") != std::string::npos){
            size_t pos = line.find("{USER}");  // 置換したい文字列が登場する位置を検索する
            size_t len = std::strlen("{USER}");    // 置換したい文字列の長さ
            line.replace(pos, len, userInfo[1]); 
        }
        content += line + "\n";
    }
    std::string response;
    response = "HTTP/1.1 200 OK\n";
    response += "Server: webserv42tokyo\n";
    response += "Date: " + getGmtDate() + "\n"; 
    response += "Content-Length: " + std::to_string(content.size()) + "\n";
    response += "Connection: Keep-Alive\n";
    response += "Content-Type: text/html\n";
    response += "\n";
    response += content;
    sendToClient(response, obj, NORMAL);
}


void HttpConnection::deleteCgiHeader(std::string &responseHeaders){
    size_t idx = responseHeaders.find("CGI-Response-type:");
    responseHeaders = responseHeaders.substr(0, idx-1);
}

std::string HttpConnection::addAnnotationToLoginPage(std::string annotation){
    std::ifstream ifs("documents/login.html");
    std::string line;
    std::string cont_len = "Content-Length: ";
    std::string content;
    while(!(ifs).eof()){
        std::getline(ifs, line);
        content += line;
        if(line.find("<h2>ログイン</h2>") != std::string::npos){
            content += "\n<p>" + annotation + "</p>" + "\n";
        }
    }
    cont_len += std::to_string(content.length()) + "\n";
    content = cont_len +"\n" + content;

    return content;
}

