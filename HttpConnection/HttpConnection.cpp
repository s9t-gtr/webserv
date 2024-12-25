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

// HttpConnection* HttpConnection::getInstance(socketSet tcpSockets){
//     HttpConnection *inst;
//     inst = new HttpConnection(tcpSockets);
//     return inst;
// }

// void HttpConnection::destroy(HttpConnection* inst){
//     delete inst;
// }

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
        // my_usleep(2000);
        for(ssize_t i = 0; i<nevent;i++){
            // std::cerr << DEBUG << BRIGHT_GREEN<< "<<<<<<<<<<<<<<< EVENT >>>>>>>>>>>>>>>" << RESET << std::endl;
            // std::cerr << DEBUG << "ident(socket or pid or timer): " << eventlist[i].ident << std::endl;

            progressInfo *obj = (progressInfo *)eventlist[i].udata;
            if(obj == NULL)
                establishTcpConnection(eventlist[i].ident, conf);
            else if(eventlist[i].filter == EVFILT_READ)
                obj->rHandler(obj);
            else if(eventlist[i].filter == EVFILT_WRITE)
                obj->wHandler(obj);
            else if(eventlist[i].filter == EVFILT_TIMER && obj->timerHandler != NULL)
                 obj->timerHandler(obj);
            else if(eventlist[i].filter == EVFILT_PROC && obj->processHandler != NULL)
                obj->processHandler(obj);
        }
    }
}

void HttpConnection::establishTcpConnection(SOCKET sockfd, Config *config){
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

    progressInfo *obj = new progressInfo(config);
    // std::memset(obj, 0, sizeof(progressInfo));
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
    obj->timerHandler = NULL;
    obj->processHandler = NULL;
    obj->socket = socket;
    std::cout << "into socket : " << socket << std::endl;
    obj->exit_status = 0;
    obj->messageTimer = false;
    obj->cgiTimer = false;
    obj->sndbuf = sndbuf;
    obj->sendKind = -1;
    obj->requestPath = "";
}


void HttpConnection::recvHandler(progressInfo *obj){
    // std::cerr << DEBUG << BRIGHT_MAGENTA BOLD << "Status: Recv" << RESET << std::endl;
    deleteTimer(obj);

    char buf[MAX_BUF_LENGTH];
    int bytesReceived = recv(obj->socket, &buf, MAX_BUF_LENGTH, MSG_DONTWAIT);
    // std::cerr << DEBUG << "bytesReceived: " << bytesReceived << std::endl;
    if(0 < bytesReceived){
        if(obj->wHandler == NULL)
            obj->requestInfo.readBufferAndParse(buf, bytesReceived);
        if(obj->requestInfo.getReadingStatus() == Request::Complete){
            
            createNewEvent(obj->socket, EVFILT_READ, EV_DELETE, 0, 0, NULL); //リクエストが完結したらREADは不要になる
            obj->rHandler = NULL;

            int responseType = obj->responseInfo.createResponse(obj);
            if(responseType == STATIC_PAGE){

                createNewEvent(obj->socket, EVFILT_WRITE, EV_ADD, 0, 0, obj);
                obj->wHandler = sendToClient;

                return ;
            }else if(responseType == CGI){
                //timerHandlerとprocessHandlerに関わるイベントを作成する
                setTimer(obj, CGI_TIMEOUT_DETECTION);

                createNewEvent(obj->responseInfo.getPidfd(), EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, obj);
                obj->processHandler = cgiExitHandler;

                return ;
            }
        }else{
            setTimer(obj, REQUEST_EOF_DETECTION);
        }
    }else if(bytesReceived == 0){
        // std::cerr << "close: socket: " << obj->socket << std::endl;
        close(obj->socket);
        // std::memset(obj, 0, sizeof(progressInfo));
        delete obj;
    }else {
        obj->sendKind = RECV;
        obj->responseInfo.createResponseFromStatusCode(500, obj->requestInfo);
        obj->wHandler = sendToClient;
    }
}


// void HttpConnection::sendCgiHandler(progressInfo *obj){
//     std::cerr << DEBUG << BLUE BOLD<< "Status: Send CGI" << RESET << std::endl;
//     obj->sendKind = NORMAL;
//     obj->httpConnection->sendToClient(obj->buffer, obj);
// }

void HttpConnection::largeResponseHandler(progressInfo *obj){
    // send()の連続呼び出しを防ぐためのhandler
    // std::cerr << DEBUG << LIGHT_GREEN BOLD << "Status: Large Response" << RESET << std::endl;
    sendToClient(obj);
}





void HttpConnection::cgiExitHandler(progressInfo *obj){
    // triggered by oneshot process event
    deleteTimer(obj);
    // std::cerr << DEBUG << PINK BOLD<< "Status: Exit Cgi " << RESET << std::endl;
    int status;
    waitpid(obj->responseInfo.getPidfd(), &status, 0);
    close(obj->responseInfo.getPipefd(W));
    if (WEXITSTATUS(status) == 0){ //cgi実行プロセスの正常終了
        obj->wHandler = readCgiHandler;
    } else {
        char buf[1024];
        ssize_t bytesReceived = read(obj->responseInfo.getPipefd(R), &buf, 1024);
        if(0 < bytesReceived)
            std::cout << std::string(buf, buf+bytesReceived) << std::endl;
        obj->responseInfo.createResponseFromStatusCode(500, obj->requestInfo);
        obj->wHandler = sendToClient;

        close(obj->responseInfo.getPipefd(R));
    }
    createNewEvent(obj->socket, EVFILT_WRITE, EV_ADD, 0, 0, obj);
}

void HttpConnection::readCgiHandler(progressInfo *obj){
    //triggered by write event
    // std::cerr << DEBUG << RED BOLD << "Status: Read CGI" << RESET << std::endl;
    char buf[MAX_BUF_LENGTH];

    ssize_t bytesReceived = read(obj->responseInfo.getPipefd(R), &buf, MAX_BUF_LENGTH);
    if(0 < bytesReceived){
        obj->buffer += std::string(buf, buf+bytesReceived);
        obj->responseInfo.readBufferAndParse(buf, bytesReceived);
        // std::cerr << DEBUG << "bytesReceived: " << bytesReceived << std::endl;
        if(obj->responseInfo.getReadingStatus() != Response::Complete){
            // obj->wHandler = sendCgiHandler;
            obj->wHandler = sendToClient;
            close(obj->responseInfo.getPipefd(R));
        }
    }else{
        // obj->sendKind = NORMAL;
        // obj->httpConnection->sendInternalErrorPage(obj);
        obj->wHandler = sendToClient;
        obj->responseInfo.createResponseFromStatusCode(500, obj->requestInfo);
    }
}

/*
    timer handler
*/

// void HttpConnection::recvEofTimerHandler(progressInfo *obj){
//     std::cerr << DEBUG << ORANGE BOLD << "Status: EofTimerHandler" << std::endl;
//     if(obj->messageTimer == true){//sendHandlerに遷移した場合は何もしない
//         obj->httpConnection->sendBadRequestPage(obj);
//     }
// }

void HttpConnection::timeoutHandler(progressInfo *obj){
    StatusCode_t statusCode = 0;
    if(obj->cgiTimer){
        statusCode = 504; 
    }else if(obj->messageTimer){
        statusCode = 400; 
    }
    obj->responseInfo.createResponseFromStatusCode(statusCode, obj->requestInfo);
    obj->wHandler = sendToClient;
}


void HttpConnection::deleteTimer(progressInfo *obj){
    /*

    */
    if(obj->messageTimer == true){
        obj->messageTimer = false;

        createNewEvent(obj->socket, EVFILT_TIMER, EV_DELETE, 0, 60000, obj);//未起動だったものは消すことでtimer大量生成を防ぐ
        obj->timerHandler = NULL; 
    }
    if(obj->cgiTimer == true){
        obj->cgiTimer = false;

        createNewEvent(obj->socket, EVFILT_TIMER, EV_DELETE, 0, 10000, obj);
        obj->timerHandler = NULL; 
    }
}


void HttpConnection::setTimer(progressInfo *obj, int type){
    if(type == REQUEST_EOF_DETECTION){
        obj->messageTimer = true;
        createNewEvent(obj->socket, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, 60000, obj);//nginx behavior(1 min timer)
    }
    if(type == CGI_TIMEOUT_DETECTION){
        obj->cgiTimer = true;
        createNewEvent(obj->socket, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, 10000, obj);
    }
    obj->timerHandler = timeoutHandler;
}


void HttpConnection::sendToClient(progressInfo *obj){
    /*
        kind: 
            normal: 0 - 通信は切断しないしwriteイベントもまだ削除していない
            cgi: 1 - cgi実行時にWRITEイベントは削除するため分岐が必要
            bad_request: 2 - objをdeleteしつつsocketもcloseするので分岐が必要
    
    */
    // std::cerr << DEBUG << "sendtoClient()" << std::endl;
    // std::cerr << DEBUG << GRAY << "======================= response =======================" << std::endl;
    // std::cerr << DEBUG << obj->buffer << RESET << std::endl;
    bool isLargeResponse = true;
    std::string response = obj->responseInfo.getResponse();
    if((size_t)obj->sndbuf < response.length()){
        obj->responseInfo.setResponse(response.substr(obj->sndbuf));
        response = response.substr(0, obj->sndbuf);
        obj->wHandler = largeResponseHandler;
    }else
        isLargeResponse = false;
    std::cout << "send() use socket : " << obj->socket << std::endl;
    std::cout << "response: " << response << std::endl;
    int status = send(obj->socket, response.c_str(), response.length(), 0);
    if (status <= 0){
        // std::cerr << DEBUG << "---- send(): error ----: " << obj->socket << std::endl;
        close(obj->socket);
        obj = NULL;
        delete obj;
        return ;
    }
    if(!isLargeResponse){//largeResponse送信途中でないのならば
        StatusCode_t statusCode = obj->responseInfo.getStatusCode();
        if(statusCode == 400 || statusCode == 500 || obj->requestInfo.getField("Connection") == "close"){//bad_requestする時にはsocketとobjを消すのでinitもreadイベント生成も不要
            // std::cerr << DEBUG << "---- close: socket ----: " << obj->socket << std::endl;
            close(obj->socket);
            delete obj;
            obj = NULL;
            return ;
        }
        // if(obj->sendKind != RECV){ //recvの時はinitだけすれば良い
        //     if(obj->sendKind != CGI_FAIL){ //writeをaddする前にsendInternalErrirに入るのでDELETEしない
        //         createNewEvent(obj->socket, EVFILT_WRITE, EV_DELETE, 0, 0, obj);
        //     }
        // }

        //次のリクエストに備える(bodyの不要なリクエスト(GET methodを持つなど)にbodyが入っていた場合、次のreadの先頭に来るのを実装するか否か迷い中)
        createNewEvent(obj->socket, EVFILT_READ, EV_ADD, 0, 0, obj);
        createNewEvent(obj->socket, EVFILT_WRITE, EV_DELETE, 0, 0, obj);
        obj->httpConnection->initProgressInfo(obj, obj->socket, obj->sndbuf);
        obj->requestInfo.clean();
        obj->responseInfo.clean();
    }   

}