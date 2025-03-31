#ifndef __HTTPCONNECTION_HPP_
# define __HTTPCONNECTION_HPP_

#include "../config/Config.hpp"
#include "../HttpMessage/Request/Request.hpp"
#include "../HttpMessage/Response/Response.hpp"
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <sstream>
#include <sys/event.h>
#include <netinet/in.h> 


class HttpConnection;

typedef struct progressInfo{
    HttpConnection* httpConnection;
    Request requestInfo;
    Response responseInfo;
    int kq;
    void (*rHandler)(progressInfo *obj);
    void (*wHandler)(progressInfo *obj);
    void (*timerHandler)(progressInfo *obj);
    void (*processHandler)(progressInfo *obj);
    std::string buffer;
    std::string::size_type content_length;
    SOCKET socket;
    int exit_status;
    pid_t childPid;
    bool messageTimer; //true: on
    bool cgiTimer;
    int sndbuf;
    int sendKind;
    std::string clientIpAddress; 
    std::string requestPath;
    char **holdMetaVariableEnviron;
    progressInfo(Config *config): requestInfo(config){}
} progressInfo;

typedef struct timespec timespec;
typedef std::map<int, struct kevent*> keventMap;


#define W 1
#define R 0
// #define TIMEOUT 10000
#define MAX_BUF_LENGTH 64
#define UPLOAD "upload/"

#define NORMAL 0
#define CGI_FAIL 1
#define BAD_REQ 2
#define RECV 3
#define FORK 4
#define SEND 5
#define CLOSE 6

#define REQUEST_EOF_DETECTION 1
#define CGI_TIMEOUT_DETECTION 2

class HttpConnection{
    private:
        static int kq;
        static struct kevent eventlist[1000];
        static timespec timeSpec;
    private:
        HttpConnection();
        HttpConnection(const HttpConnection& other);
        HttpConnection& operator=(const HttpConnection& other);
    public:
        HttpConnection(socketSet tcpSockets);
        ~HttpConnection();
        // static HttpConnection* getInstance(socketSet tpcSockets);
        // static void destroy(HttpConnection *inst);
        void startEventLoop(Config *conf);
    private:
        void connectionPrepare(socketSet tcpSockets);
        void createKqueue();
        void createEventOfEstablishTcpConnection(socketSet tcpSockets);
        static void createNewEvent(SOCKET targetSocket, short filter, u_short flags, u_int fflags, intptr_t data, void *obj);
        static void eventRegister(struct kevent *changelist);

        void eventExecute(Config *conf, SOCKET sockefd, socketSet tcpSockets);
        void establishTcpConnection(SOCKET sockfd, Config *config);
        void sendResponse(progressInfo *obj);
        // void executeCgi(Request& requestInfo, int *pipe_c2p);

        static void sendToClient(progressInfo *obj);
        // std::string getStringFromHtml(std::string wantHtmlPath);
        // void sendDefaultErrorPage(VirtualServer* server, progressInfo *obj);
        // void sendAutoindexPage(Request& requestInfo, progressInfo *obj);
        // std::string createAutoindexPage(Request& requestInfo, std::string path);
        // static std::string getGmtDate();
        // void sendStaticPage(progressInfo *obj);
        // void createResponse(std::string statusLine, std::string content);
        // void sendRedirectPage(Location* location, progressInfo *obj);
        void postProcess(Request& requestInfo, progressInfo *obj);
        // void executeCgi_postVersion(Request& requestInfo, int pipe_c2p[2]);
        // void sendForbiddenPage(progressInfo *obj);
        void deleteProcess(progressInfo *obj);
        // static void sendNotImplementedPage(progressInfo *obj);
        // static void sendNotAllowedPage(progressInfo *obj);
        // static void requestEntityPage(progressInfo *obj);


        // bool isAllowedMethod(Location* location, std::string method);
        // static void sendInternalErrorPage(progressInfo *obj);
        // static void sendBadRequestPage(progressInfo *obj);
        // static void sendFoundPage(progressInfo *obj);
        // bool isReadNewLine(std::string buffer);
        // bool bodyConfirm(progressInfo info);
        // bool checkCompleteRecieved(progressInfo info);

        void initProgressInfo(progressInfo *obj, SOCKET socket, int sndbuf, std::string clientIpAddress);
        static void recvHandler(progressInfo *obj);
        // static void recvEofTimerHandler(progressInfo *obj);
        static void timeoutHandler(progressInfo *obj);
        // static void sendHandler(progressInfo *obj);
        static void cgiExitHandler(progressInfo *obj);
        static void readCgiHandler(progressInfo *obj);
        // static void sendCgiHandler(progressInfo *obj);
        // static void sendTimeoutPage(progressInfo *obj);
        static void largeResponseHandler(progressInfo *obj);
        // static void confirmExitStatusFromCgi(progressInfo *obj);

        //cookie/sessions
        // void cookiePage(Request& requestInfo, progressInfo *obj);
        // static std::string createRedirectPath(std::string requestPath);
        // void sendLoginPage(int status, Request& requestInfo, progressInfo *obj);
        // void sendUserPage(std::vector<std::string> userInfo, int status, Request& requestInfo, progressInfo *obj);
        // static void deleteCgiHeader(std::string &responseHeaders);
        // static std::string addAnnotationToLoginPage(std::string annotation);


        //timer 
        static void setTimer(progressInfo *obj, int type);
        static void deleteTimer(progressInfo *obj);
};

#endif

