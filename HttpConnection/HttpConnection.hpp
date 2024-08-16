#ifndef __HTTPCONNECTION_HPP_
# define __HTTPCONNECTION_HPP_

#include "../config/Config.hpp"
#include "../request/RequestParse.hpp"
#include <dirent.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <sstream>
#include <sys/event.h>
class HttpConnection;
typedef struct progressInfo{
    HttpConnection* httpConnection;
    int kq;
    void (*rHandler)(progressInfo *obj);
    void (*wHandler)(progressInfo *obj, Config *conf);
    void (*tHandler)(progressInfo *obj);
    void (*pHandler)(progressInfo *obj);
    bool readCgiInitial;
    std::string buffer;
    std::string::size_type content_length;
    int pipe_c2p[2];
    SOCKET socket;
    int exit_status;
    pid_t childPid;
    bool eofTimer; //true: on
    int sndbuf;
    int tmpKind;
    std::string requestPath;

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

class HttpConnection{
    private:
        static int kq;
        static struct kevent eventlist[1000];
        static timespec timeSpec;
    private:
        HttpConnection();
        HttpConnection(socketSet tcpSockets);
        HttpConnection(const HttpConnection& other);
        HttpConnection& operator=(const HttpConnection& other);
        ~HttpConnection();
    public:
        static HttpConnection* getInstance(socketSet tpcSockets);
        static void destroy(HttpConnection *inst);
        void startEventLoop(Config *conf);
    private:
        static void connectionPrepare(socketSet tcpSockets);
        static void createKqueue();
        static void createEventOfEstablishTcpConnection(socketSet tcpSockets);
        static void createNewEvent(SOCKET targetSocket, short filter, u_short flags, u_int fflags, intptr_t data, void *obj);
        static void eventRegister(struct kevent *changelist);
        void eventExecute(Config *conf, SOCKET sockefd, socketSet tcpSockets);
        void establishTcpConnection(SOCKET sockfd);
        void sendResponse(RequestParse& requestInfo, progressInfo *obj);
        void executeCgi(RequestParse& requestInfo, int *pipe_c2p);

        void sendToClient(std::string response, progressInfo *obj, int kind);
        static std::string getStringFromHtml(std::string wantHtmlPath);
        void sendDefaultErrorPage(VirtualServer* server, progressInfo *obj);
        void sendAutoindexPage(RequestParse& requestInfo, progressInfo *obj);
        std::string createAutoindexPage(RequestParse& requestInfo, std::string path);
        std::string getGmtDate();
        void sendStaticPage(RequestParse& requestInfo, progressInfo *obj);
        void createResponse(std::string statusLine, std::string content);
        void sendRedirectPage(Location* location, progressInfo *obj);
        void postProcess(RequestParse& requestInfo, progressInfo *obj);
        void executeCgi_postVersion(RequestParse& requestInfo, int pipe_c2p[2]);
        void sendForbiddenPage(progressInfo *obj);
        void deleteProcess(RequestParse& requestInfo, progressInfo *obj);
        void sendNotImplementedPage(progressInfo *obj);
        void sendNotAllowedPage(progressInfo *obj);
        void requestEntityPage(progressInfo *obj);


        bool isAllowedMethod(Location* location, std::string method);
        void sendInternalErrorPage(progressInfo *obj, int kind);
        void sendBadRequestPage(progressInfo *obj);

        bool isReadNewLine(std::string buffer);
        bool bodyConfirm(progressInfo info);
        bool checkCompleteRecieved(progressInfo info);

        void initProgressInfo(progressInfo *obj, SOCKET socket, int sndbuf);
        static void recvHandler(progressInfo *obj);
        static void recvEofTimerHandler(progressInfo *obj);

        static void sendHandler(progressInfo *obj, Config *conf);
        static void readCgiHandler(progressInfo *obj, Config *conf);
        static void sendCgiHandler(progressInfo *obj, Config *conf);
        static void sendTimeoutPage(progressInfo *obj);
        static void sendLargeResponse(progressInfo *obj, Config *conf);
        static void confirmExitStatusFromCgi(progressInfo *obj);

        //cookie/sessions
        void cookiePage(RequestParse& requestInfo, progressInfo *obj);
        static std::string createRedirectPath(std::string requestPath);

        void sendLoginPage(int status, RequestParse& requestInfo, progressInfo *obj);
        void sendUserPage(std::vector<std::string> userInfo, int status, RequestParse& requestInfo, progressInfo *obj);
        static void deleteCgiHeader(std::string &responseHeaders);
        static std::string addAnnotationToLoginPage(std::string annotation);


};

#endif

