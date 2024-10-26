#include "HttpConnection.hpp"

// void HttpConnection::sendToClient(progressInfo *obj){
//     /*
//         kind: 
//             normal: 0 - 通信は切断しないしwriteイベントもまだ削除していない
//             cgi: 1 - cgi実行時にWRITEイベントは削除するため分岐が必要
//             bad_request: 2 - objをdeleteしつつsocketもcloseするので分岐が必要
    
//     */
//     std::cerr << DEBUG << "sendtoClient()" << std::endl;
//     std::cerr << DEBUG << GRAY << "======================= response =======================" << std::endl;
//     std::cerr << DEBUG << obj->buffer << RESET << std::endl;
//     bool isLargeResponse = false;
//     std::string response = obj->responseInfo.getResponse();
//     if((size_t)obj->sndbuf < response.length()){
//         obj->responseInfo.setResponse(response.substr(obj->sndbuf));
//         response = response.substr(0, obj->sndbuf);
//         obj->wHandler = largeResponseHandler;
//     }else
//         isLargeResponse = true;

//     int status = send(obj->socket, response.c_str(), response.length(), 0);
//     if (status == 0){
//         perror("send error: client connection close");
//         obj->sendKind = 2;
//     } 
//     else if (status < 0){
//         if(obj->sendKind == SEND)//send()の失敗による obj->sendKind=SENDときのsendInternalErrorPage()送信時のsend()でも失敗したら
//             obj->sendKind = BAD_REQ;
//         else{
//             obj->sendKind = SEND;
//             sendInternalErrorPage(obj); 
//         }
//     }
//     if(!isLargeResponse){//largeResponse送信途中でないのならば
//         StatusCode_t statusCode = obj->responseInfo.getStatusCode();
//         if(statusCode == 400 || statusCode == 500 || obj->requestInfo.getField("Connection") == "close"){//bad_requestする時にはsocketとobjを消すのでinitもreadイベント生成も不要
//             std::cerr << DEBUG << "---- close: socket ----: " << obj->socket << std::endl;
//             close(obj->socket);
//             obj = NULL;
//             delete obj;
//             return ;
//         // }
//         // if(obj->sendKind != RECV){ //recvの時はinitだけすれば良い
//         //     if(obj->sendKind != CGI_FAIL){ //writeをaddする前にsendInternalErrirに入るのでDELETEしない
//         //         createNewEvent(obj->socket, EVFILT_WRITE, EV_DELETE, 0, 0, obj);
//         //     }
//         // }

//         //次のリクエストに備える(bodyの不要なリクエストにbodyが入っていた場合、次のreadの先頭に来るのを実装するか否か迷い中)
//         createNewEvent(obj->socket, EVFILT_READ, EV_ADD, 0, 0, obj);
//         obj->httpConnection->initProgressInfo(obj, obj->socket, obj->sndbuf);
//     }
    
        
// }

// サーバーからのレスポンスヘッダーに含まれるGMT時刻を取得する関数
// std::string HttpConnection::getGmtDate()
// {
//     std::time_t now = std::time(nullptr);
//     std::tm* tm_now = std::gmtime(&now);
//     char buffer[80];
//     std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm_now);
    
//     return (std::string(buffer));
// }

// サーバーからのレスポンスとしてリダイレクトページを送る関数
// void HttpConnection::sendRedirectPage(Location* location, progressInfo *obj)
// {
//     std::string response;
//     response = "HTTP/1.1 301 Moved Permanently\n";
//     response += "Connection: Keep-Alive\n";
//     response += "Content-Length: 0\n";
//     response += "Date: " + getGmtDate() + "\n"; 
//     response += "Server: webserv/1.0.0\n"; 
//     response += "Location: " + location->locationSetting["return"] + "\n";
//     response += "\n";//ヘッダーとボディを分けるために、ボディが空でも必要
//     obj->sendKind = NORMAL;
//     sendToClient(response, obj);

// }

// サーバーからのレスポンスとしてデフォルトのエラーページを送る関数
// void HttpConnection::sendDefaultErrorPage(VirtualServer* server, progressInfo *obj)
// {
//     std::string error_page_path = server->serverSetting["error_page"];
//     // デフォルトのエラーページの内容を取得
//     std::ifstream file(error_page_path);
//     if (!file.is_open()) {
//         //ファイルパスが無効ならデフォルトを設定
//         error_page_path = "documents/404_default.html";
//         file.close();  // 既存のファイルストリームを閉じる
//         file.open(error_page_path);  // 新しいファイルを開く
//     }
//     std::string line;
//     std::string content;
//     while (std::getline(file, line)) {
//         content += line;
//         content += "\n";
//     }
//     file.close();

//     std::string response;
//     response = "HTTP/1.1 404 Not Found\n";
//     response += "Server: webserv42tokyo\n";
//     response += "Date: " + getGmtDate() + "\n"; 
//     response += "Content-Length: " + std::to_string(content.size()) + "\n";
//     response += "Connection: Keep-Alive\n";
//     response += "Content-Type: text/html\n";
//     response += "\n";
//     response += content;
//     obj->sendKind = NORMAL;
//     sendToClient(response, obj);
// }

// サーバーからのレスポンスとして静的ファイルを送る関数
// void HttpConnection::sendStaticPage(progressInfo *obj)
// {
//     std::string file_path = obj->requestInfo.getPath();
//     Location *location = obj->requestInfo.getLocation();
//     VirtualServer *server = obj->requestInfo.getServer();
    
//     if(file_path == "documents/user.html" || file_path == "documents/login.html"){
//         return cookiePage(obj->requestInfo, obj);
//     }
        
    // std::ifstream file(file_path);
    // if (!file.is_open())
    //     return sendDefaultErrorPage(server, obj);//404エラー <- std::exitのほうがいい？
    // std::string line;
    // std::string content;
    // while (std::getline(file, line)) {
    //     content += line;
    //     content += "\n";
    // }
    // file.close();

    // std::string response;
    // response = "HTTP/1.1 200 OK\n";
    // response += "Server: webserv42tokyo\n";
    // response += "Date: " + getGmtDate() + "\n"; 
    // response += "Content-Length: " + std::to_string(content.size()) + "\n";
    // response += "Connection: Keep-Alive\n";
    // response += "Content-Type: text/html\n";
    // response += "\n";
    // response += content;
    // if(obj->requestInfo.getField("Connection") == "close"){
    //     obj->sendKind = CLOSE;
    //     return sendToClient(response, obj);
    // }
    // obj->sendKind = NORMAL;
    // sendToClient(response, obj);
// }

// void HttpConnection::sendFoundPage(progressInfo *obj)
// {
//     std::string response;
//     response = "HTTP/1.1 302 Moved Temporarily\n";
//     response += "Connection: Keep-Alive\n";
//     response += "Content-Length: 0\n";
//     response += "Date: " + getGmtDate() + "\n"; 
//     response += "Server: webserv/1.0.0\n"; 
//     response += "Location: " + obj->requestInfo.getServer()->serverSetting["error_page"] + "\n";
//     response += "\n";//ヘッダーとボディを分けるために、ボディが空でも必要
//     obj->sendKind = RECV;
//     sendToClient(response, obj);

// }

// bool HttpConnection::checkCompleteRecieved(progressInfo obj){
//     std::string::size_type head = 0;
//     std::string::size_type tail = obj.buffer.find("\n");
//     while(obj.content_length == 0 && tail != std::string::npos){
//         std::string line = obj.buffer.substr(head, tail);
//         std::string::size_type target = line.find("Content-Length:");
//         if(target != std::string::npos){
//             std::stringstream ss;
//             ss << line.substr(target+15);
//             ss >> obj.content_length;
//         }
//         head = tail + 1;
//         tail = obj.buffer.find("\n", head);
//     }
//     if(isReadNewLine(obj.buffer) && obj.content_length == 0){
//         // std::cerr << DEBUG << "Complete recieved" << std::endl;
//         return true; //get method && no body request
//     }
//     else if(isReadNewLine(obj.buffer) && bodyConfirm(obj)){
//         // std::cerr << DEBUG << "Complete recieved" << std::endl;
//         return true; // already read body of request
//     }
//     return false;
// }

// bool HttpConnection::isReadNewLine(std::string buffer){
//     if(buffer.find("\n\r\n") != std::string::npos || buffer.find("\n\n") != std::string::npos)
//         return true;
//     return false;
// }

// bool HttpConnection::bodyConfirm(progressInfo info){
//     size_t returnLen = std::strlen("\r\n\r\n");
//     std::string::size_type newLineIdx = info.buffer.find("\r\n\r\n");
//     if(newLineIdx == std::string::npos){
//         returnLen = std::strlen("\n\n");
//         newLineIdx = info.buffer.find("\n\n");
//     }
//     std::string tmpBody = info.buffer.substr(newLineIdx+returnLen);
//     if(tmpBody.size() == info.content_length)
//         return true;
//     return false;
// }






// void HttpConnection::cookiePage(Request& requestInfo, progressInfo *obj){
//     std::string cookieInfo = requestInfo.getField("Cookie");
//     if(requestInfo.getPath() == "documents/user.html"){
//         if(cookieInfo != "" && requestInfo.searchSessionId(cookieInfo)){
//             std::vector<std::string> vec =  requestInfo.getUserInfo(cookieInfo);
//             sendUserPage(vec, 200, requestInfo, obj);//cgiでhtmlを成形し、responseヘッダも繋げたものを返す
//         }else{
//             sendLoginPage(303, requestInfo, obj);
//         }
//     }else if (requestInfo.getPath() == "documents/login.html"){
//         if(cookieInfo != "" && requestInfo.searchSessionId(cookieInfo)){
//             std::vector<std::string> vec =  requestInfo.getUserInfo(cookieInfo);
//             sendUserPage(vec, 303, requestInfo, obj);//cgiでhtmlを成形し、responseヘッダも繋げたものを返す
//         }else{
//             sendLoginPage(200, requestInfo, obj);
//         }
//     }
// }
// std::string HttpConnection::createRedirectPath(std::string requestPath){
//     size_t idx = requestPath.rfind("/");
//     std::string path = requestPath.substr(0, idx+1);
//     if(requestPath.find("user.html") != std::string::npos){
//         path += "login.html";
//     }else if(requestPath.find("login.html") != std::string::npos){
//         path += "user.html";
//     }else{
//         std::cerr << DEBUG << "storange: createRedirectPath()" << std::endl;
//     }
//     return path;
// }

// void HttpConnection::sendLoginPage(int status, Request& requestInfo, progressInfo *obj){
//     if(status == 303){
//         std::string redirectPath = createRedirectPath(requestInfo.getPath());
//         std::string response = "HTTP/1.1 303 See Other\n";
//         response += "Connection: Keep-Alive\n";
//         response += "Content-Length: "+ std::to_string(std::strlen("Login successful")) + "\n";
//         response += "Date: " + getGmtDate() + "\n";
//         response += "Server: webserv/1.0.0\n";
//         response += "Location: " + redirectPath + "\n";
//         response += "\n";
//         response += "Login successful";
//         obj->sendKind = NORMAL;
//         sendToClient(response, obj);
//         return ;
//     }
//     std::string content = getStringFromHtml("documents/login.html");
//     std::string response;
//     response = "HTTP/1.1 200 OK\n";
//     response += "Server: webserv42tokyo\n";
//     response += "Date: " + getGmtDate() + "\n"; 
//     response += "Content-Length: " + std::to_string(content.size()) + "\n";
//     response += "Connection: Keep-Alive\n";
//     response += "Content-Type: text/html\n";
//     response += "\n";
//     response += content;
//     obj->sendKind = NORMAL;
//     sendToClient(response, obj);
// }

// void HttpConnection::sendUserPage(std::vector<std::string> userInfo, int status, Request& requestInfo, progressInfo *obj){
//     /*
//         htmlファイルをuser情報に書き換え、responseヘッダも連結した文字列を出力するCGI
//     */
//    if(status == 303){
//         std::string redirectPath = createRedirectPath(requestInfo.getPath());
//         std::string response = "HTTP/1.1 303 See Other\n";
//         response += "Connection: Keep-Alive\n";
//         response += "Content-Length: "+ std::to_string(std::strlen("Login successful")) + "\n";
//         response += "Date: " + getGmtDate() + "\n";
//         response += "Server: webserv/1.0.0\n";
//         response += "Location: " + redirectPath + "\n";
//         response += "\n";
//         response += "Login successful";
//         obj->sendKind = NORMAL;
//         sendToClient(response, obj);
//         return ;
//    }
//    std::ifstream ifs("documents/user.html");
//    if(!ifs){
//         perror("sendUserPage: documents/user.html");
//    }
//    std::string line;
//    std::string content;
//     while(!(ifs).eof()){
//         std::getline(ifs, line);
//         if(line.find("Welcome") != std::string::npos){
//             size_t pos = line.find("{USER}");  // 置換したい文字列が登場する位置を検索する
//             size_t len = std::strlen("{USER}");    // 置換したい文字列の長さ
//             line.replace(pos, len, userInfo[1]); 
//         }
//         content += line + "\n";
//     }
//     std::string response;
//     response = "HTTP/1.1 200 OK\n";
//     response += "Server: webserv42tokyo\n";
//     response += "Date: " + getGmtDate() + "\n"; 
//     response += "Content-Length: " + std::to_string(content.size()) + "\n";
//     response += "Connection: Keep-Alive\n";
//     response += "Content-Type: text/html\n";
//     response += "\n";
//     response += content;
//     obj->sendKind = NORMAL;
//     sendToClient(response, obj);
// }


// // void HttpConnection::deleteCgiHeader(std::string &responseHeaders){
// //     size_t idx = responseHeaders.find("CGI-Response-type:");
// //     responseHeaders = responseHeaders.substr(0, idx-1);
// // }

// std::string HttpConnection::addAnnotationToLoginPage(std::string annotation){
//     std::ifstream ifs("documents/login.html");
//     std::string line;
//     std::string cont_len = "Content-Length: ";
//     std::string content;
//     while(!(ifs).eof()){
//         std::getline(ifs, line);
//         content += line;
//         if(line.find("<h2>ログイン</h2>") != std::string::npos){
//             content += "\n<p>" + annotation + "</p>" + "\n";
//         }
//     }
//     cont_len += std::to_string(content.length()) + "\n";
//     content = cont_len +"\n" + content;

//     return content;
// }
