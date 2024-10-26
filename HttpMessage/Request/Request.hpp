#ifndef __REQUEST_H_
# define __REQUEST_H_

#include <sstream>
#include <sys/stat.h>
#include "../../config/Config.hpp"
#include "../HttpMessageParser.hpp"
// #include "../../HttpConnection/HttpConnection.hpp"

using namespace std;

#define ALIVE true
#define DEAD false
#define EXIST_VERSION true
#define NOT_EXIST_VERSION false


class Request: public HttpMessageParser{
    public:
    public:
        HttpMessageParser::DetailStatus::StartLineReadStatus createInitialStatus();
        Request(Config *config);
        virtual ~Request();

        virtual void parseStartLine(char c);
        virtual void checkIsWatingLF(char c, bool isExistThirdElement);
        
        bool getIsCgi();
        bool getIsAutoindex();

        string getMethod();
        string getPath();
        string getRequestTarget();
        string getVersion();
        string getHostName();
        string getPort();
        VirtualServer *getServer();
        Location *getLocation();
        bool searchSessionId(string cookieInfo);
        vector<string> getUserInfo(string cookieInfo);

        StatusCode_t confirmRequest();
    private: 
        void setConfigInfo();
        StatusCode_t validateRequestTarget();


        void setCorrespondServer();
        void setCorrespondLocation();
        Location* getLocationSetting();
        void createPathFromConfRoot();
        
        bool isAllowMethod();
        string selectBestMatchLocation(map<string, Location*> &locations);

        bool isCgiRequest();
    
        //cookie/sessions
        string getSessionId(string cookieInfo);
        
    private:
        // Request-specific elements
        Config *config;
        string method;
        string requestTarget;
        string version;
        string pathFromConfRoot;

        bool isCgi;
        bool isAutoindex;
        // unsigned long long bodySize;
        
        VirtualServer* server;
        Location* location;
        bool allocFlag;
};


#endif