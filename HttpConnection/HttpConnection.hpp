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

struct tmpInfo{
    enum Status{
        Recv,
        Send
    };
    Status status;
    std::string tmpBuffer;
    std::string::size_type content_length;
};

typedef struct timespec timespec;
typedef std::map<int, struct kevent*> keventMap;
typedef std::map<int, tmpInfo> tmpInfoMap;

#define W 1
#define R 0
#define MAX_BUF_LENGTH 11
#define UPLOAD "/upload/"

class HttpConnection{
    private:
        static int kq;
        static keventMap events;
        static tmpInfoMap tmpInfos;
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
        void startEventLoop(Config *conf, socketSet tcpSockets);
    private:
        static void connectionPrepare(socketSet tcpSockets);
        static void createKqueue();
        static void createTcpConnectionEvents(socketSet tcpSockets);
        static void createNewEvent(SOCKET targetSocket);
        static void eventRegister(SOCKET fd);
        void eventExecute(Config *conf, SOCKET sockefd, socketSet tcpSockets);
        void establishTcpConnection(SOCKET sockfd);
        // bool isExistBuffer(SOCKET sockfd);
        void requestHandler(Config *conf, SOCKET sockfd);
        void sendResponse(Config *conf, RequestParse& requestInfo, SOCKET sockfd);
        void executeCgi(Config *conf, RequestParse& requestInfo, int *pipe_c2p);
        void createResponseFromCgiOutput(pid_t pid, SOCKET sockfd, int pipe_c2p[2]);

        void sendDefaultErrorPage(SOCKET sockfd, VirtualServer* server);
        void sendAutoindexPage(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server, Location* location);
        std::string getGmtDate();
        void sendStaticPage(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server, Location* location);
        void sendRedirectPage(SOCKET sockfd, Location* location);
        void postProcess(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server);
        void executeCgi_postVersion(RequestParse& requestInfo, int pipe_c2p[2]);
        void sendForbiddenPage(SOCKET sockfd);
        void deleteProcess(RequestParse& requestInfo, SOCKET sockfd, VirtualServer* server);
        void sendNotImplementedPage(SOCKET sockfd);
        void sendNotAllowedPage(SOCKET sockfd);
        void requestEntityPage(SOCKET sockfd);
        std::string selectLocationSetting(std::map<std::string, Location*> &locations, std::string request_path);
        bool isAllowedMethod(Location* location, std::string method);
        void sendTimeoutPage(SOCKET sockfd);
        void sendInternalErrorPage(SOCKET sockfd);

        bool isReadNewLine(std::string tmpBuffer);
        bool bodyConfirm(tmpInfo info);
        bool checkCompleteRecieved(tmpInfo info);
        void createConnectEvent(SOCKET targetSocket);
};

#endif