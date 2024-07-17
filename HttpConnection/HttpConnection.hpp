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
    std::string buffer;
    std::string::size_type content_length;
    int pipe_c2p[2];
    SOCKET socket;
    // int timeout;
    int exit_status;
    pid_t childPid;
    bool eofTimer; //true: on

} progressInfo;

typedef struct timespec timespec;
typedef std::map<int, struct kevent*> keventMap;

// typedef std::map<int, progressInfo> progressInfoMap;

#define W 1
#define R 0
// #define TIMEOUT 10000
#define MAX_BUF_LENGTH 64
#define UPLOAD "/upload/"

class HttpConnection{
    private:
        static int kq;
        // static progressInfoMap progressInfos;
        static struct kevent *eventlist;
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
        // bool isExistBuffer(SOCKET sockfd);
        // void requestHandler(Config *conf, SOCKET sockfd);
        void sendResponse(RequestParse& requestInfo, SOCKET sockfd, progressInfo *obj);
        void executeCgi(RequestParse& requestInfo, int *pipe_c2p);

        void sendDefaultErrorPage(SOCKET sockfd, VirtualServer* server);
        void sendAutoindexPage(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server, Location* location);
        void sendToClient(SOCKET sockfd, std::string response);
        std::string getGmtDate();
        void sendStaticPage(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server, Location* location);
        void createResponse(std::string statusLine, std::string content);
        void sendRedirectPage(SOCKET sockfd, Location* location);
        void postProcess(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server, progressInfo *obj);
        void executeCgi_postVersion(RequestParse& requestInfo, int pipe_c2p[2]);
        void sendForbiddenPage(SOCKET sockfd);
        void deleteProcess(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server);
        void sendNotImplementedPage(SOCKET sockfd);
        void sendNotAllowedPage(SOCKET sockfd);
        void requestEntityPage(SOCKET sockfd);


        bool isAllowedMethod(Location* location, std::string method);
        void sendInternalErrorPage(SOCKET sockfd);
        void sendBadRequestPage(SOCKET sockfd);

        bool isReadNewLine(std::string buffer);
        bool bodyConfirm(progressInfo info);
        bool checkCompleteRecieved(progressInfo info);

        void initProgressInfo(progressInfo *obj, SOCKET socket);
        static void recvHandler(progressInfo *obj);
        static void recvEofTimerHandler(progressInfo *obj);

        static void sendHandler(progressInfo *obj, Config *conf);
        static void readCgiHandler(progressInfo *obj, Config *conf);
        static void sendCgiHandler(progressInfo *obj, Config *conf);
        static void sendTimeoutPage(progressInfo *obj);
        static void confirmExitStatusFromCgi(progressInfo *obj);
};

#endif

