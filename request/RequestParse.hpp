#ifndef __REQUESTPARSE_H_
# define __REQUESTPARSE_H_

#include "../config/Config.hpp"

#define ALIVE true;
#define DEAD false;

typedef std::map<std::string, std::string> headersMap;

class RequestParse{
    private:
        RequestParse();
    public:
        RequestParse(std::string requestMessage, Config *conf);
        ~RequestParse();
        RequestParse(const RequestParse& other);
        RequestParse& operator=(const RequestParse& other);
    public:
        std::string getMethod();
        std::string getPath();
        std::string getRawPath();
        std::string getVersion();
        std::string getHeader(std::string header);
        std::string getBody();
        std::string getHostName();
        std::string getPort();
        VirtualServer *getServer();
        Location *getLocation();

    private: 
        std::string getRequestLine(std::string& requestMessage);
        void setMethodPathVersion(std::string& requestMessage);
        void setHeadersAndBody(std::string& requestMessage);
        void setHeaders(std::vector<std::string> linesVec, std::vector<std::string>::iterator& it);
        std::vector<std::string> splitLines(std::string str, char sep);
        void setBody(std::vector<std::string> linesVec, std::vector<std::string>::iterator itFromBody);
        std::string createBodyStringFromLinesVector(std::vector<std::string> linesVec);
        void bodyUnChunk(std::vector<std::string> linesVec, std::vector<std::string>::iterator itFromBody);
        void setCorrespondServer(Config *conf);
        void setCorrespondLocation();
        Location* getLocationSetting();
        std::string selectBestMatchLocation(std::map<std::string, Location*> &locations);
        void createPathFromConfRoot();
    private:

    private:
        std::string method;
        std::string rawPath;
        std::string pathFromConfRoot;
        std::string version;
        headersMap headers;
        std::string body;
        VirtualServer* server;
        Location* location;
        bool allocFlag;

    public:
        void test__headers();
};

#endif