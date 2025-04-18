#include "HttpConnection.hpp"

int HttpConnection::kq;
struct kevent HttpConnection::eventlist[1000];
timespec HttpConnection::timeSpec = {0,0};
// timespec HttpConnection::timeSpec = {0,100000000}
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

void my_usleep(useconds_t usec){
    clock_t st = std::clock();
    clock_t et = st +(usec * CLOCKS_PER_SEC / 1000000);
    while(std::clock() < et){;}
}
void HttpConnection::startEventLoop(Config *conf){
    while(1){
        ssize_t nevent = kevent(kq, NULL, 0, eventlist, sizeof(*eventlist), &timeSpec);
        my_usleep(2000);
        for(ssize_t i = 0; i<nevent;i++){
            // std::cerr << DEBUG << BRIGHT_GREEN<< "<<<<<<<<<<<<<<< EVENT >>>>>>>>>>>>>>>" << RESET << std::endl;
            // std::cerr << DEBUG << "ident(socket or pid or timer): " << eventlist[i].ident << std::endl;

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
    // std::cerr << DEBUG << "TCP CONNECTION ESTABLISHED " << std::endl;

    struct sockaddr_storage client_sa;
    socklen_t len = sizeof(client_sa);   
    int newSocket = accept(sockfd, (struct sockaddr*) &client_sa, &len);
    if (newSocket < 0)
    {
        perror("accept");
        printf("errno = %d (%s)\n", errno, strerror(errno));
        throw std::runtime_error("Error: accept() failed()");
    }
    // std::cerr << DEBUG << "[sokcet:" << sockfd << "] create new socket: " << newSocket << std::endl;
    
    int sndbuf = 1;
    len = sizeof(sndbuf);
    getsockopt(newSocket, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len);
    // std::cerr << DEBUG << "sndbuf: " << sndbuf << std::endl;//NOTICE: これ以上の長さのレスポンスを返すためにSend()もloopすべきかも

    int rcvbuf = 1;
    len = sizeof(rcvbuf);
    getsockopt(newSocket, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
    // std::cerr << DEBUG << "rcvbuf: " << rcvbuf << std::endl;//NOTICE: これ以上の長さのレスポンスを返すためにSend()もloopすべきかも

    progressInfo *obj = new progressInfo();
    std::memset(obj, 0, sizeof(progressInfo));
    initProgressInfo(obj, newSocket, sndbuf);
    createNewEvent(newSocket, EVFILT_READ, EV_ADD, 0, 0, obj);
}

void HttpConnection::initProgressInfo(progressInfo *obj, SOCKET socket, int sndbuf){
    obj->kq = kq;
    obj->buffer = "";
    obj->content_length = 0;
    obj->httpConnection = this;
    obj->rHandler = recvHandler;
    obj->wHandler = NULL;
    obj->tHandler = NULL;
    obj->pHandler = NULL;
    obj->readCgiInitial = true;
    obj->socket = socket;
    obj->exit_status = 0;
    obj->eofTimer = false;
    obj->sndbuf = sndbuf;
    obj->requestPath = "";
    obj->isClose = false;
}

void HttpConnection::recvHandler(progressInfo *obj){
    // std::cerr << DEBUG << BRIGHT_MAGENTA BOLD << "Status: Recv" << RESET << std::endl;
    if(obj->eofTimer == true){
        createNewEvent(obj->socket, EVFILT_TIMER, EV_DELETE, 0, 3000, obj);//未起動だったものは消すことでtimer大量生成を防ぐ
    }
    char buf[MAX_BUF_LENGTH];
    int bytesReceived = recv(obj->socket, &buf, MAX_BUF_LENGTH, MSG_DONTWAIT);
    // std::cerr << DEBUG << "bytesReceived: " << bytesReceived << std::endl;
    if(0 < bytesReceived){
        if(obj->buffer.size()+bytesReceived < obj->buffer.max_size()){
            obj->buffer += std::string(buf, buf+bytesReceived);
            if(obj->httpConnection->checkCompleteRecieved(*obj)){
                obj->wHandler = sendHandler;
                obj->eofTimer = false;
                obj->tHandler = NULL;
                createNewEvent(obj->socket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                createNewEvent(obj->socket, EVFILT_WRITE, EV_ADD, 0, 0, obj);
            }else{
                obj->eofTimer = true;
                obj->tHandler = recvEofTimerHandler;
                createNewEvent(obj->socket, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, 3000, obj);
            }
        }else
            obj->httpConnection->sendBadRequestPage(obj);
    }else if(bytesReceived == 0){
        // std::cerr << "close: socket: " << obj->socket << std::endl;
        if(obj != NULL){
            close(obj->socket);
            delete obj;
        }
    }else
        obj->wHandler = sendInternalErrorPage;
}

void HttpConnection::recvEofTimerHandler(progressInfo *obj){
    // std::cerr << DEBUG << ORANGE BOLD << "Status: EofTimerHandler" << std::endl;
    if(obj->eofTimer == true){//sendHandlerに遷移した場合は何もしない
        obj->httpConnection->sendBadRequestPage(obj);
    }
}

void createQueryString(RequestParse *requestInfo) {
    std::string path = requestInfo->getPath();
    std::string::size_type idx = path.find("?");
    if(idx == std::string::npos) { return ; }
    requestInfo->setPathFromConfRoot(path.substr(0, idx));
    requestInfo->setQueryString(path.substr(idx+1));
}
void HttpConnection::sendHandler(progressInfo *obj, Config *conf){
    // std::cerr << DEBUG << LIGHT_BLUE BOLD <<  "Status: Send" << RESET << std::endl;
    try{
        RequestParse requestInfo(obj->buffer, conf); //例外発生元はこれ
        obj->requestPath = requestInfo.getPath();
        createQueryString(&requestInfo);
        obj->httpConnection->sendResponse(requestInfo, obj);
    }catch(std::runtime_error){
        return obj->httpConnection->sendBadRequestPage(obj);
    }
}

void HttpConnection::readCgiHandler(progressInfo *obj, Config *conf){
    // std::cerr << DEBUG << RED BOLD << "Status: Read CGI" << RESET << std::endl;
    if(obj->readCgiInitial){
        obj->readCgiInitial = false;
        createNewEvent(obj->socket, EVFILT_TIMER, EV_DELETE, 0, 10000, obj);
    }
    (void)conf;//wHandlerのインターフェース的に必要
    char buf[MAX_BUF_LENGTH];

    ssize_t bytesReceived = read(obj->pipe_c2p[R], &buf, MAX_BUF_LENGTH);
    if(0 < bytesReceived){
        obj->buffer += std::string(buf, buf+bytesReceived);
        // std::cerr << DEBUG << "bytesReceived: " << bytesReceived << std::endl;
        if(obj->httpConnection->checkCompleteRecieved(*obj)){
            obj->wHandler = sendCgiHandler;
            close(obj->pipe_c2p[R]);
        }
    }else
        obj->wHandler = sendInternalErrorPage;
        // obj->httpConnection->sendInternalErrorPage(obj, NORMAL);
}

void HttpConnection::sendCgiHandler(progressInfo *obj, Config *conf){
    (void)conf;
    // std::cerr << DEBUG << BLUE BOLD<< "Status: Send CGI" << RESET << std::endl;
    obj->httpConnection->sendToClient(obj->buffer, obj);
}

void HttpConnection::sendLargeResponse(progressInfo *obj, Config *conf){
    (void)conf;
    // std::cerr << DEBUG << LIGHT_GREEN BOLD<< "Status: Large Response" << RESET << std::endl;
    obj->httpConnection->sendToClient(obj->buffer, obj);
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

bool isCgi(RequestParse& requestInfo)
{   
    std::string path = requestInfo.getPath();
    std::string::size_type pathLength = path.size();
    const std::string pythonExtension = ".py";
    const std::string cgiExtension = ".cgi";
    if(path.find(".") == std::string::npos)
        return false;
    if(path.substr(pathLength > cgiExtension.size() ? pathLength - cgiExtension.size() : 0) == cgiExtension)
        return true;
    if(path.substr(pathLength > pythonExtension.size() ? pathLength - pythonExtension.size() : 0) == pythonExtension)
        return true;

    return false;
}

void HttpConnection::sendResponse(RequestParse& requestInfo, progressInfo *obj){
    VirtualServer *server = requestInfo.getServer();
    Location *location = requestInfo.getLocation();
    std::string path = requestInfo.getPath();
    if (requestInfo.getMethod() == "GET")
    {
        // allow_methodが設定され、かつGETが含まれていなかった場合
        if (isAllowedMethod(location, "GET") == false)
            return sendNotAllowedPage(obj);
        // redirec ->location設定の中で最優先
        if (location->locationSetting["return"] != "none")
            sendRedirectPage(location, obj);
        //autoindex ->sendStaticPage関数内
        else if (isCgi(requestInfo))//<- .cgi実行ファイルもMakefileで作成・削除できるようにする
        {
            if (access(path.c_str(), F_OK) != 0)
                return sendDefaultErrorPage(server, obj);
            if (access(path.c_str(), X_OK))
                return sendForbiddenPage(obj);
            int pipe_c2p[2];
            if(pipe(pipe_c2p) < 0)
                throw std::runtime_error("Error: pipe() failed");
            obj->pipe_c2p[R] = pipe_c2p[R];
            obj->pipe_c2p[W] = pipe_c2p[W];
            obj->tHandler = sendTimeoutPage;
            obj->pHandler = confirmExitStatusFromCgi;
            pid_t pid = fork();
            if(pid == 0){
                executeCgi(obj, requestInfo, pipe_c2p);
            } else if(pid > 0) {
                obj->childPid = pid;
                // std::cerr << DEBUG << "obj->childPid: " << obj->childPid << std::endl;
                createNewEvent(obj->socket, EVFILT_WRITE, EV_DELETE, 0, 0, obj);
                createNewEvent(obj->socket, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, 10000, obj);
                createNewEvent(pid, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, obj);
            } else {
                close(pipe_c2p[R]);
                close(pipe_c2p[W]);
                sendInternalErrorPage(obj, NULL);
            }
        }
        //その他の静的ファイルまたはディレクトリ
        else
            sendStaticPage(requestInfo, obj);
    }
    else if (requestInfo.getMethod() == "POST")
    {
        if (isAllowedMethod(location, "POST") == false)
            return sendNotAllowedPage(obj);
        else if(requestInfo.getPath() == UPLOAD)
            postProcess(requestInfo, obj);
        else if(isCgi(requestInfo))
            postProcess(requestInfo, obj);
        else//メソッドがPOSTなのにリクエストパスがcgi以外の場合
            sendForbiddenPage(obj);
    }
    else if (requestInfo.getMethod() == "DELETE")
    {
        if (isAllowedMethod(location, "DELETE") == false)
            return sendNotAllowedPage(obj);
        if (path.substr(0, 7) == UPLOAD)// リクエストパスがアップロード可能なディレクトリならファイルの削除
            deleteProcess(requestInfo, obj);
        else//メソッドがDELETEなのにリクエストパスが"/upload/"以外の場合
            sendForbiddenPage(obj);
    }
    else //GET,POST,DELETE以外の実装していないメソッド
        sendNotImplementedPage(obj);
}

void HttpConnection::createEnviron(progressInfo *obj, RequestParse &requestInfo, char **environ){
    MetaVariables metaVariables;

    size_t i = 0;
    size_t numEnvironLine = 0;
    while(environ[i++] != NULL){
        numEnvironLine++;
    }
    obj->holdMetaVariableEnviron = (char **)std::malloc((numEnvironLine + metaVariables.count() + 1) * sizeof(char *));
    i = 0;
    while(i < numEnvironLine){
        obj->holdMetaVariableEnviron[i] = strdup(environ[i]);
        i++;
    }
    for (std::map<std::string, EnvironFunction>::iterator it = metaVariables.variablesToFuncPtr.begin(); it!=metaVariables.variablesToFuncPtr.end(); it++){
        it->second(obj, requestInfo, i++);
    }
    obj->holdMetaVariableEnviron[i] = NULL;
    
}
void HttpConnection::executeCgi(progressInfo *obj, RequestParse &requestInfo,  int pipe_c2p[2]){
    close(pipe_c2p[R]);
    dup2(pipe_c2p[W],1);
    close(pipe_c2p[W]);
    std::string script = requestInfo.getPath();
    if(script.size() > 3 && script.substr(0, 2) == "./") {
        script = script.substr(2);
    }
    extern char** environ;
    createEnviron(obj, requestInfo, environ);
    // std::cerr << "=== Environment Variables Before execve ===\n";
    // for (char **env = obj->holdMetaVariableEnviron; *env; ++env) {
    //     std::cerr << *env << std::endl;
    // }
    // std::cerr << "===========================================\n";
    // std::cerr << script << std::endl;
    if(script.substr(script.length() - 3) == ".py"){
		const char* script_c = script.c_str();
        char* const cgi_argv[] = { NULL };
        if(execve(script_c, cgi_argv, obj->holdMetaVariableEnviron) < 0){
			perror("execve");
            exit(1);
		}
    }
    const char* cPath = script.c_str();
    // std::cout << "cPath: " << cPath << std::endl;
    char* const cgi_argv[] = { const_cast<char*>(cPath), NULL };
    if(execve(cPath, cgi_argv, obj->holdMetaVariableEnviron) < 0){
        exit(1);
    }
}

void HttpConnection::confirmExitStatusFromCgi(progressInfo *obj){
    obj->tHandler = NULL;
    // std::cerr << DEBUG << PINK BOLD<< "Status: Exit Cgi " << RESET << std::endl;
    int status;
    waitpid(obj->childPid, &status, 0);
    close(obj->pipe_c2p[W]);
    if (WEXITSTATUS(status) == 0){
        obj->buffer = "";
        obj->wHandler = readCgiHandler;
        createNewEvent(obj->socket, EVFILT_WRITE, EV_ADD, 0, 0, obj);
    } else
        obj->httpConnection->sendInternalErrorPage(obj, NULL);
}
