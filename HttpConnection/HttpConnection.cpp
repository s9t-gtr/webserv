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
    std::cout << "TCP CONNECTION ESTABLISHED"<< std::endl;

    struct sockaddr_storage client_sa;  // sockaddr_in 型ではない。 
    socklen_t len = sizeof(client_sa);   
    int newSocket = accept(sockfd, (struct sockaddr*) &client_sa, &len);
    //----------acceptのエラー処理追加------------
    if (newSocket < 0)
    {
        perror("accept");
        std::exit(EXIT_FAILURE);
    }
    //----------acceptのエラー処理追加------------
    createNewEvent(newSocket);
    eventRegister(newSocket);
    std::cout << "event socket = " << sockfd << std::endl;
    std::cout << "new socket = " << newSocket << std::endl;
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
    else if (bytesReceived == 0) //read/recv/write/sendが失敗したら返り値を0と-1で分けて処理する。その後クライアントをremoveする。
        close(sockfd); //返り値が0のときは接続の失敗
    else 
    {
        perror("recv error"); //返り値が-1のときはシステムコールの失敗
        close(sockfd);
        std::exit(EXIT_FAILURE);
    }
    //----------recvのエラー処理追加------------
}

void HttpConnection::sendResponse(Config *conf, RequestParse& requestInfo, SOCKET sockfd){
    // -------------リクエストごとに振り分ける処理を追加-------------
    if (requestInfo.getMethod() == "GET")
    {
        //redirect
        if (requestInfo.getPath() == "/google")
            sendRedirectPage(sockfd);
        //autoindex
        else if (requestInfo.getPath().substr(0, 11) == "/autoindex/")
            sendAutoindexPage(requestInfo, sockfd);
        //cgi
        else if (requestInfo.getPath() == "/cgi/test.cgi")//<- .cgi実行ファイルもMakefileで作成・削除できるようにする
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
            sendStaticPage(requestInfo, sockfd);
    }
    else if (requestInfo.getMethod() == "POST")
    {
        if (requestInfo.getPath() == "/upload/")// リクエストパスがアップロード可能なディレクトリならファイルの作成
            postProcess(requestInfo, sockfd);
        else//メソッドがPOSTなのにリクエストパスが"/upload/"以外の場合
            sendForbiddenPage(sockfd);
    }
    else if (requestInfo.getMethod() == "DELETE")
    {
        if (requestInfo.getPath().substr(0, 8) == "/upload/")// リクエストパスがアップロード可能なディレクトリならファイルの削除
            deleteProcess(requestInfo, sockfd);
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
    extern char** environ;
    VirtualServer server = conf->getServer(requestInfo.getHostName());
    std::string cgiPath = server.getCgiPath();
    char* const cgi_argv[] = { const_cast<char*>(cgiPath.c_str()), NULL };
    if(execve("../cgi/test.cgi", cgi_argv, environ) < 0)
        std::cout << "Error: execve() failed" << std::endl;
}

void HttpConnection::createResponseFromCgiOutput(pid_t pid, SOCKET sockfd, int pipe_c2p[2]){
    char res_buf[MAX_BUF_LENGTH];
    waitpid(pid, NULL, 0);
    close(pipe_c2p[W]);
    ssize_t byte = read(pipe_c2p[R], &res_buf, MAX_BUF_LENGTH);
    if(byte > 0){
        res_buf[byte] = '\0';
        if(send(sockfd, &res_buf, byte, 0) < 0)
            std::cerr << "Error: send() failed" << std::endl;
        else
            std::cout << "send!!!!!!" << std::endl;
    }
}