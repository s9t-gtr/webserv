#include "HttpConnection.hpp"

int HttpConnection::kq;
keventMap HttpConnection::events;
struct kevent *HttpConnection::eventlist;
timespec HttpConnection::timeSpec = {0,1000000000};

HttpConnection::HttpConnection(){}
HttpConnection::~HttpConnection(){}
HttpConnection::HttpConnection(socketSet tcpSockets){
    connectionPrepare(tcpSockets);
}
/*========================================
        public member functions
========================================*/
HttpConnection* HttpConnection::getInstance(socketSet tcpSockets){
    HttpConnection *inst;
    inst = new HttpConnection(tcpSockets);
    return inst;
}

void HttpConnection::destroy(HttpConnection* inst){
    delete inst;
}

void HttpConnection::connectionPrepare(socketSet tcpSockets){
    createKqueue();
    createTcpConnectionEvents(tcpSockets);
    eventlist = new struct kevent[tcpSockets.size()*6]; //最近のブラウザは最大6つのtcpコネクションを確立するらしいのでtcpSockets*6倍のイベント容量を用意する
}

void HttpConnection::createKqueue(){
    kq = kqueue();
    if(kq == -1){
        // for(socketSet::const_iterator it=tcpSockets.begin();it==tcpSockets.end();it++)
        //     close(*it);
        throw std::runtime_error("Error: kqueue() failed()"); 
    }
}
void HttpConnection::createTcpConnectionEvents(socketSet tcpSockets){
    for(socketSet::const_iterator it=tcpSockets.begin();it!=tcpSockets.end();it++){
        createNewEvent(*it);
        eventRegister(*it);
    }
}

void HttpConnection::createNewEvent(SOCKET targetSocket){
    events[targetSocket] = new struct kevent;
    EV_SET(events[targetSocket], targetSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);
}

void HttpConnection::eventRegister(SOCKET fd){
    if(kevent(kq, events[fd], 1, NULL, 0, NULL) == -1){ // ポーリングするためにはtimeout引数を非NULLのtimespec構造体ポインタを指す必要がある
        perror("kevent"); // kevent: エラーメッセージ
        printf("errno = %d (%s)\n", errno, strerror(errno)); 
        // for(socketSet::const_iterator it=tcpSockets.begin();it!=tcpSockets.end();it++)
        //     close(*it);
        // for(socketSet::const_iterator it=tcpSockets.begin();*it!=fd;it++)
        //     delete events[*it];
        // delete events[fd];//現在のループで作った分
        throw std::runtime_error("Error: kevent() failed()");
    }
}

void HttpConnection::startEventLoop(Config *conf, socketSet tcpSockets){
    while(1){
        //---------size_tをssize_tに修正-----------//
        ssize_t nevent = kevent(kq, NULL, 0, eventlist, sizeof(*eventlist), NULL);
        for(ssize_t i = 0; i<nevent;i++){
            // usleep(1100);
            eventExecute(conf, eventlist[i].ident, tcpSockets);
        }
        //---------size_tをssize_tに修正-----------//
    }
}

void HttpConnection::eventExecute(Config *conf, SOCKET sockfd, socketSet tcpSockets){
    if (tcpSockets.find(sockfd) != tcpSockets.end()) 
        establishTcpConnection(sockfd);
    else
        requestHandler(conf, sockfd);
}

void HttpConnection::establishTcpConnection(SOCKET sockfd){
    // std::cout << "TCP CONNECTION ESTABLISHED"<< std::endl;

    struct sockaddr_storage client_sa;  // sockaddr_in 型ではない。 
    socklen_t len = sizeof(client_sa);   
    int newSocket = accept(sockfd, (struct sockaddr*) &client_sa, &len);
    //----------acceptのエラー処理追加------------
    if (newSocket < 0)
    {
        perror("accept");
        return;
    }
    //----------acceptのエラー処理追加------------
    createNewEvent(newSocket);
    eventRegister(newSocket);
    // std::cout << "event socket = " << sockfd << std::endl;
    // std::cout << "new socket = " << newSocket << std::endl;
}

void HttpConnection::requestHandler(Config *conf, SOCKET sockfd){
    char buf[MAX_BUF_LENGTH];
    int bytesReceived = recv(sockfd, &buf, MAX_BUF_LENGTH, MSG_DONTWAIT);
    // std::cout << "bytesRecieved: " << bytesReceived << std::endl;
    if(bytesReceived > 0){
        // std::cout << "==========EVENT==========" << std::endl;
        // std::cout << "event socket = " << sockfd << std::endl;
        // std::cout << "HTTP REQUEST"<< std::endl;
        std::string request = std::string(buf, buf+bytesReceived);
        // std::cout << buf << std::endl;
        RequestParse requestInfo(request);
        sendResponse(conf, requestInfo, sockfd);
    }   

    //----------recvのエラー処理追加------------
    else if (bytesReceived == 0){
        delete events[sockfd];
        close(sockfd); //返り値が0のときは接続の失敗
    } //read/recv/write/sendが失敗したら返り値を0と-1で分けて処理する。その後クライアントをremoveする。
    else 
    {
        perror("recv error"); //返り値が-1のときはシステムコールの失敗
        delete events[sockfd];
        close(sockfd);
        // std::exit(EXIT_FAILURE);
    }
    //----------recvのエラー処理追加------------
}

std::string HttpConnection::selectLocationSetting(std::map<std::string, Location*> &locations, std::string request_path)
{
    std::string bestMatch = "";
    for (std::map<std::string, Location*>::const_iterator it = locations.begin(); it != locations.end(); ++it)
    {
        if (it->second->locationSetting.find("locationPath") != it->second->locationSetting.end())
        {
            const std::string &locationPath = it->second->locationSetting["locationPath"];
            if (request_path.substr(0, locationPath.size()) == locationPath)
            {
                if (locationPath.size() > bestMatch.size())
                    bestMatch = locationPath;
            }
        }
    }
    return (bestMatch);
}

bool HttpConnection::isAllowedMethod(Location* location, std::string method)
{
    if(location->locationSetting["allow_method"] == "none")
        return true;
    std::vector<std::string>status = split(location->locationSetting["allow_method"], ' ');
    for (std::vector<std::string>::iterator it = status.begin(); it != status.end(); ++it)
    {
        if (*it == method)
            break;
        if (it == status.end() - 1)
            return false;
    }
    return true;
}

void handleTimeout(int sig)
{
    (void)sig;
    std::cerr << "Error: CGI script execution timed out" << std::endl;
    // 必要ならば他のクリーンアップ処理をここに追加
    std::exit(1);
}

bool isCgi(RequestParse& requestInfo)
{
    std::string path = requestInfo.getPath();
    
    // パスの長さが4以上か確認
    if (path.length() >= 4) {
        // 最後の4文字を取得
        std::string lastFourChars = path.substr(path.length() - 4);
        
        // 最後の4文字が ".cgi" であるか確認
        if (lastFourChars == ".cgi") {
            // ここに処理を追加
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void HttpConnection::sendResponse(Config *conf, RequestParse& requestInfo, SOCKET sockfd){
    //今回指定されたバーチャルサーバーの設定情報を使いたいのでインスタンス化
    VirtualServer* server = conf->getServer(requestInfo.getHostName());

    //今回指定されたlocationパスの設定情報を使いたいのでインスタンス化
    std::string location_path = selectLocationSetting(server->locations, requestInfo.getPath());
    Location* location = server->locations[location_path];

    // std::cout << "========" << location->path << "========" << std::endl;//デバッグ

    // -------------リクエストごとに振り分ける処理を追加-------------
    if (requestInfo.getMethod() == "GET")
    {
        // allow_methodが設定され、かつGETが含まれていなかった場合
        if (isAllowedMethod(location, "GET") == false)
        {
            sendNotAllowedPage(sockfd);
            return ;
        }
        // redirec ->location設定の中で最優先
        if (location->locationSetting["return"] != "none")
            sendRedirectPage(sockfd, location);
        //autoindex ->sendStaticPage関数内に移動
        // else if (requestInfo.getPath().substr(0, 11) == "/autoindex/")
        //     sendAutoindexPage(requestInfo, sockfd, server, location);
        //cgi
        else if (isCgi(requestInfo) == true)//<- .cgi実行ファイルもMakefileで作成・削除できるようにする
        {
            int pipe_c2p[2];
            if(pipe(pipe_c2p) < 0)
                throw std::runtime_error("Error: pipe() failed");
            pid_t pid = fork();
            if(pid == 0)
                executeCgi(conf, requestInfo, pipe_c2p);
            createResponseFromCgiOutput(pid, sockfd, pipe_c2p);
        }
        //その他の静的ファイルまたはディレクトリ
        else
            sendStaticPage(requestInfo, sockfd, server, location);
    }
    else if (requestInfo.getMethod() == "POST")
    {
        // allow_methodが設定され、かつPOSTが含まれていなかった場合
        if (isAllowedMethod(location, "POST") == false)
        {
            sendNotAllowedPage(sockfd);
            return ;
        }
        if (requestInfo.getPath() == UPLOAD)// リクエストパスがアップロード可能なディレクトリならファイルの作成
            postProcess(requestInfo, sockfd, server);
        else//メソッドがPOSTなのにリクエストパスが"/upload/"以外の場合
            sendForbiddenPage(sockfd);
    }
    else if (requestInfo.getMethod() == "DELETE")
    {
        // allow_methodが設定され、かつDELETEが含まれていなかった場合
        if (isAllowedMethod(location, "DELETE") == false)
        {
            sendNotAllowedPage(sockfd);
            return ;
        }
        if (requestInfo.getPath().substr(0, 8) == UPLOAD)// リクエストパスがアップロード可能なディレクトリならファイルの削除
            deleteProcess(requestInfo, sockfd, server);
        else//メソッドがDELETEなのにリクエストパスが"/upload/"以外の場合
            sendForbiddenPage(sockfd);
    }
    else //GET,POST,DELETE以外の実装していないメソッド
        sendNotImplementedPage(sockfd);
    // -------------リクエストごとに振り分ける処理を追加-------------
}

void HttpConnection::executeCgi(Config *conf, RequestParse& requestInfo, int pipe_c2p[2]){
    close(pipe_c2p[R]);
    dup2(pipe_c2p[W],1);
    close(pipe_c2p[W]);

    signal(SIGALRM, handleTimeout);
    alarm(5);

    extern char** environ;
    VirtualServer* server = conf->getServer(requestInfo.getHostName());
    std::string cgiPath = server->getCgiPath();
    char* const cgi_argv[] = { const_cast<char*>(cgiPath.c_str()), NULL };

    std::string path = ".." + requestInfo.getPath();
    const char* cPath = path.c_str();

    if(execve(cPath, cgi_argv, environ) < 0)
        std::cout << "Error: execve() failed" << std::endl;
}

void HttpConnection::createResponseFromCgiOutput(pid_t pid, SOCKET sockfd, int pipe_c2p[2]){
    char res_buf[MAX_BUF_LENGTH];
    int status;
    waitpid(pid, &status, 0);
    close(pipe_c2p[W]);

    int exit_status;
	if (WIFSIGNALED(status) != 0)
		exit_status = WTERMSIG(status);
	else
		exit_status = WEXITSTATUS(status);

    if (exit_status == 1)
    {
        sendInternalErrorPage(sockfd);
        return ;
    }

    if (exit_status == 14)
    {
        sendTimeoutPage(sockfd);
        return ;
    }

    ssize_t byte = read(pipe_c2p[R], &res_buf, MAX_BUF_LENGTH);
    if (byte == 0)
    {
        delete events[sockfd];
        close(sockfd);
    }
    if (byte == -1)
    {
        perror("read error"); //返り値が-1のときはシステムコールの失敗
        delete events[sockfd];
        close(sockfd);
        // std::exit(EXIT_FAILURE);
    }
    if(byte > 0){
        res_buf[byte] = '\0';
        int status = send(sockfd, &res_buf, byte, 0);
        if (status == 0){
            delete events[sockfd];
            close(sockfd); //返り値が0のときは接続の失敗
        } //read/recv/write/sendが失敗したら返り値を0と-1で分けて処理する。その後クライアントをremoveする。
        if (status < 0)
        {
            perror("send error"); //返り値が-1のときはシステムコールの失敗
            delete events[sockfd];
            close(sockfd);
            // std::exit(EXIT_FAILURE);
        }
    }
}
