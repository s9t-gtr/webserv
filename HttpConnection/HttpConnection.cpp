#include "HttpConnection.hpp"

int HttpConnection::kq;
// keventMap HttpConnection::changelist;
struct kevent *HttpConnection::eventlist;
timespec HttpConnection::timeSpec = {0,0};
// timespec HttpConnection::timeSpec = {0,1000000000};
// progressInfoMap HttpConnection::progressInfo;

HttpConnection::HttpConnection(){}
HttpConnection::~HttpConnection(){}
HttpConnection::HttpConnection(socketSet tcpSockets){
    connectionPrepare(tcpSockets);
}

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
    createEventOfEstablishTcpConnection(tcpSockets);
    eventlist = new struct kevent[tcpSockets.size()*6]; //最近のブラウザは最大6つのtcpコネクションを確立するらしいのでtcpSockets*6倍のイベント容量を用意する
}

void HttpConnection::createKqueue(){
    kq = kqueue();
    if(kq == -1){
        throw std::runtime_error("Error: kqueue() failed()"); 
    }
}
void HttpConnection::createEventOfEstablishTcpConnection(socketSet tcpSockets){
    for(socketSet::const_iterator it=tcpSockets.begin();it!=tcpSockets.end();it++){
        createNewEvent(*it, EVFILT_READ, EV_ADD, 0, 0,  NULL);
    }
}

void HttpConnection::createNewEvent(SOCKET targetSocket, short filter, u_short flags, u_int fflags, intptr_t data, void *obj){
    struct kevent changelist[1];
    EV_SET(changelist, targetSocket, filter, flags, fflags, data, obj);
    eventRegister(changelist);
}

void HttpConnection::eventRegister(struct kevent *changelist){
    if(kevent(kq, changelist, 1, NULL, 0, NULL) == -1){ // ポーリングするためにはtimeout引数を非NULLのtimespec構造体ポインタを指す必要がある
        perror("kevent"); // kevent: エラーメッセージ
        printf("errno = %d (%s)\n", errno, strerror(errno)); 
    }
}

void HttpConnection::startEventLoop(Config *conf){
    while(1){
        ssize_t nevent = kevent(kq, NULL, 0, eventlist, sizeof(*eventlist), &timeSpec);
        for(ssize_t i = 0; i<nevent;i++){
            std::cerr << "<<<<<<<<<<<<<<< EVENT >>>>>>>>>>>>>>>" << std::endl;
            std::cerr << "eventlist.ident: " << eventlist[i].ident << std::endl;

            progressInfo *obj = (progressInfo *)eventlist[i].udata;
            if(obj == NULL)
                establishTcpConnection(eventlist[i].ident);
            else if(eventlist[i].filter == EVFILT_READ)
                obj->rHandler(obj);
            else if(eventlist[i].filter == EVFILT_WRITE)
                obj->wHandler(obj, conf);
            else if(eventlist[i].filter == EVFILT_TIMER && obj->tHandler != NULL)
                obj->tHandler(obj);
            else if(eventlist[i].filter == EVFILT_PROC && obj->pHandler != NULL)
                obj->pHandler(obj);
        }
    }
}

void HttpConnection::establishTcpConnection(SOCKET sockfd){
    std::cerr << "TCP CONNECTION ESTABLISHED: socket: " << sockfd << std::endl;

    struct sockaddr_storage client_sa;
    socklen_t len = sizeof(client_sa);   
    int newSocket = accept(sockfd, (struct sockaddr*) &client_sa, &len);
    if (newSocket < 0)
    {
        perror("accept");
        printf("errno = %d (%s)\n", errno, strerror(errno));
        throw std::runtime_error("Error: accept() failed()");
    }
    int rcvbuf = 1;
    len = sizeof(rcvbuf);
    getsockopt(newSocket, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
    std::cout << "rcvbuf: " << rcvbuf << std::endl;//NOTICE: これ以上の長さのレスポンスを返すためにSend()もloopすべきかも
    std::cerr << "create connection socket: " << newSocket << std::endl;
    progressInfo *obj = new progressInfo();
    std::memset(obj, 0, sizeof(progressInfo));
    initProgressInfo(obj, newSocket);
    createNewEvent(newSocket, EVFILT_READ, EV_ADD, 0, 0, obj);
}

void HttpConnection::initProgressInfo(progressInfo *obj, SOCKET socket){
    obj->kq = kq;
    obj->buffer = "";
    obj->content_length = 0;
    obj->httpConnection = this;
    obj->rHandler = recvHandler;
    obj->wHandler = NULL;
    obj->tHandler = NULL;
    obj->pHandler = NULL;
    obj->socket = socket;
    obj->exit_status = 0;
    obj->eofTimer = false;
}

void HttpConnection::recvHandler(progressInfo *obj){
    std::cerr << "Status: Recv" << std::endl;
    if(obj->eofTimer == true){
        createNewEvent(obj->socket, EVFILT_TIMER, EV_DELETE, 0, 3000, obj);//未起動だったものは消すことでtimer大量生成を防ぐ
    }
    char buf[MAX_BUF_LENGTH];
    int bytesReceived = recv(obj->socket, &buf, MAX_BUF_LENGTH, MSG_DONTWAIT);
    if(0 < bytesReceived){
        obj->buffer += std::string(buf, buf+bytesReceived);
        if(obj->httpConnection->checkCompleteRecieved(*obj)){
            std::cout << obj->buffer << std::endl;
            obj->wHandler = sendHandler;
            obj->eofTimer = false;
            obj->tHandler = NULL;
            createNewEvent(obj->socket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
            createNewEvent(obj->socket, EVFILT_WRITE, EV_ADD, 0, 0, obj);
            std::cout << obj->buffer << std::endl;
        }else{
            obj->eofTimer = true;
            obj->tHandler = recvEofTimerHandler;
            createNewEvent(obj->socket, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, 3000, obj);
        }
    }else{
        std::cerr << "close: socket: " << obj->socket << std::endl;
        close(obj->socket);
        delete obj;
    }
}

void HttpConnection::recvEofTimerHandler(progressInfo *obj){
    std::cerr << "Status: EofTimerHandler" << std::endl;
    if(obj->eofTimer == true){//sendHandlerに遷移した場合は何もしない
        createNewEvent(obj->socket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        std::cout << obj->buffer << std::endl;
        obj->httpConnection->sendBadRequestPage(obj->socket);
        std::cerr << "close: socket: " << obj->socket << std::endl;
        close(obj->socket);
        delete obj;
    }
}

void HttpConnection::sendHandler(progressInfo *obj, Config *conf){
    // std::cerr << "Status: Send" << std::endl;
    try{
        RequestParse requestInfo(obj->buffer, conf);
        obj->httpConnection->sendResponse(requestInfo, obj->socket, obj);
    }catch(std::runtime_error){
        obj->httpConnection->sendBadRequestPage(obj->socket);
        std::cerr << "close: socket: " << obj->socket << std::endl;
        close(obj->socket);
        delete obj;
        return ;
    }
    if(obj->wHandler == sendHandler && obj->tHandler != sendTimeoutPage){
        createNewEvent(obj->socket, EVFILT_WRITE, EV_DELETE, 0, 0, obj);
        obj->httpConnection->initProgressInfo(obj, obj->socket);
        createNewEvent(obj->socket, EVFILT_READ, EV_ADD, 0, 0, obj);
        std::cerr << "<<<<<<<<<< Status: Send finish >>>>>>>>>>>" << std::endl;
    }
}

void HttpConnection::readCgiHandler(progressInfo *obj, Config *conf){
    // std::cerr << "Status: Read CGI" << std::endl;
    (void)conf;
    char buf[MAX_BUF_LENGTH];

    ssize_t bytesReceived = read(obj->pipe_c2p[R], &buf, MAX_BUF_LENGTH);
    if(0 < bytesReceived){
        obj->buffer += std::string(buf, buf+bytesReceived);
        // std::cerr << "read bytesReceived: " << bytesReceived << std::endl;
        if(obj->httpConnection->checkCompleteRecieved(*obj)){
            std::cerr <<obj->socket <<  ": Status: Read CGI finish" << std::endl;
            obj->wHandler = sendCgiHandler;
            close(obj->pipe_c2p[R]);
        }
    }else{
        perror("read error"); //返り値が-1のときはシステムコールの失敗
        obj->httpConnection->sendInternalErrorPage(obj->socket);
        obj->httpConnection->initProgressInfo(obj, obj->socket);
        createNewEvent(obj->socket, EVFILT_READ, EV_ADD, 0, 0, obj);
        createNewEvent(obj->socket, EVFILT_WRITE, EV_DELETE, 0, 0, obj);

    }
}

void HttpConnection::sendCgiHandler(progressInfo *obj, Config *conf){
    (void)conf;
    std::cerr << "Status: Send CGI" << std::endl;
    obj->httpConnection->sendToClient(obj->socket, obj->buffer);
    createNewEvent(obj->socket, EVFILT_WRITE, EV_DELETE, 0, 0, obj);
    obj->httpConnection->initProgressInfo(obj, obj->socket);
    createNewEvent(obj->socket, EVFILT_READ, EV_ADD, 0, 0, obj);
}


bool HttpConnection::isAllowedMethod(Location* location, std::string method)
{
    if(location->locationSetting["allow_method"] == "none"){
        return true;
    }
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

bool isCgi(RequestParse& requestInfo)
{
    std::string path = requestInfo.getPath();
    if (path.length() >= 4) {
        std::string lastFourChars = path.substr(path.length() - 4);
        
        if (lastFourChars == ".cgi") {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void HttpConnection::sendResponse(RequestParse& requestInfo, SOCKET sockfd, progressInfo *obj){
    VirtualServer *server = requestInfo.getServer();
    Location *location = requestInfo.getLocation();
    if (requestInfo.getMethod() == "GET")
    {
        // std::cerr << "DEBUG: GET()" << std::endl;
        // allow_methodが設定され、かつGETが含まれていなかった場合
        if (isAllowedMethod(location, "GET") == false)
        {
            //std::cerr << "DEBUG: isAllowMethod() out" << std::endl;
            sendNotAllowedPage(sockfd);
            return ;
        }
        // redirec ->location設定の中で最優先
        if (location->locationSetting["return"] != "none")
            sendRedirectPage(sockfd, location);
        //autoindex ->sendStaticPage関数内に移動
        // else if (requestInfo.getPath().substr(0, 11) == "/autoindex/")
        //     sendAutoindexPage(requestInfo, sockfd, server, location);
        else if (isCgi(requestInfo))//<- .cgi実行ファイルもMakefileで作成・削除できるようにする
        {
            std::string path = requestInfo.getPath();
            if (access(path.c_str(), F_OK) != 0)
                return sendDefaultErrorPage(sockfd, server);
            if (access(path.c_str(), X_OK))
                return sendForbiddenPage(sockfd);

            int pipe_c2p[2];
            if(pipe(pipe_c2p) < 0)
                throw std::runtime_error("Error: pipe() failed");
            obj->pipe_c2p[R] = pipe_c2p[R];
            obj->pipe_c2p[W] = pipe_c2p[W];
            obj->tHandler = sendTimeoutPage;
            obj->pHandler = confirmExitStatusFromCgi;
            pid_t pid = fork();
            if(pid == 0){
                executeCgi(requestInfo, pipe_c2p);
            } else if(pid > 0) {
                obj->childPid = pid;
                createNewEvent(obj->socket, EVFILT_WRITE, EV_DELETE, 0, 0, obj);
                createNewEvent(obj->socket, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, 10000, obj);
                createNewEvent(pid, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, obj);
            } else {
                perror("fork() error");
            }
        }
        //その他の静的ファイルまたはディレクトリ
        else{
            std::cout << "DEBUG : autoindex: " << location->searchSetting("autoindex")->second << std::endl;
            sendStaticPage(requestInfo, sockfd, server, location);
        }
    }
    else if (requestInfo.getMethod() == "POST")
    {
        // allow_methodが設定され、かつPOSTが含まれていなかった場合
        if (isAllowedMethod(location, "POST") == false)
        {
            sendNotAllowedPage(sockfd);
            return ;
        }
        if (requestInfo.getRawPath() == UPLOAD)// リクエストパスがアップロード可能なディレクトリならファイルの作成
            postProcess(requestInfo, sockfd, server, obj);
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
}


void HttpConnection::executeCgi(RequestParse& requestInfo, int pipe_c2p[2]){
    close(pipe_c2p[R]);
    dup2(pipe_c2p[W],1);
    close(pipe_c2p[W]);
    extern char** environ;
    std::string path = requestInfo.getPath();
    char* const cgi_argv[] = { const_cast<char*>(path.c_str()), NULL };
    const char* cPath = path.c_str();
    if(execve(cPath, cgi_argv, environ) < 0)
        exit(1);
}

void HttpConnection::confirmExitStatusFromCgi(progressInfo *obj){
    obj->tHandler = NULL;
    std::cerr << "Status: Exit Cgi " << std::endl;
    int status;
    waitpid(obj->childPid, &status, 0);
    close(obj->pipe_c2p[W]);
    if (WEXITSTATUS(status) == 0){
        obj->buffer = "";
        obj->wHandler = readCgiHandler;
        createNewEvent(obj->socket, EVFILT_WRITE, EV_ADD, 0, 0, obj);
    } else if(WEXITSTATUS(status) == 1){
        //DEBUG: 
        obj->buffer = "";
        obj->wHandler = readCgiHandler;
        createNewEvent(obj->socket, EVFILT_WRITE, EV_ADD, 0, 0, obj);
    } else 
        obj->httpConnection->sendInternalErrorPage(obj->socket);
}
