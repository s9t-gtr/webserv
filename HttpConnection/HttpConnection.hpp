#ifndef __HTTPCONNECTION_HPP_
# define __HTTPCONNECTION_HPP_

#include "../config/Config.hpp"
#include "../request/RequestParse.hpp"
#include <dirent.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct timespec timespec;
typedef std::map<int, struct kevent*> keventMap;

#define W 1
#define R 0
#define MAX_BUF_LENGTH 4096

class HttpConnection{
    private:
        static int kq;
        static keventMap events;
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
        void requestHandler(Config *conf, SOCKET sockfd);
        void sendResponse(Config *conf, RequestParse& requestInfo, SOCKET sockfd);
        void executeCgi(Config *conf, RequestParse& requestInfo, int *pipe_c2p);
        void createResponseFromCgiOutput(pid_t pid, SOCKET sockfd, int pipe_c2p[2]);

        void sendDefaultErrorPage(SOCKET sockfd);
        void sendAutoindexPage(RequestParse& requestInfo, SOCKET sockfd);
        std::string getGmtDate();
        void sendStaticPage(RequestParse& requestInfo, SOCKET sockfd);
        void sendRedirectPage(SOCKET sockfd);
        void postProcess(RequestParse& requestInfo, SOCKET sockfd);
        void executeCgi_postVersion(RequestParse& requestInfo, int pipe_c2p[2]);
        void sendForbiddenPage(SOCKET sockfd);
        void deleteProcess(RequestParse& requestInfo, SOCKET sockfd);
        void sendNotImplementedPage(SOCKET sockfd);
};

#endif