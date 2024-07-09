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
        RequestParse(std::string requestMessage);
        ~RequestParse();
        RequestParse(const RequestParse& other);
        RequestParse& operator=(const RequestParse& other);
    public:
        std::string getMethod();
        std::string getPath();
        std::string getVersion();
        std::string getHeader(std::string header);
        std::string getBody();
        std::string getHostName();
    private: 
        std::string getRequestLine(std::string& requestMessage);
        void setMethodPathVersion(std::string& requestMessage);
        void setHeadersAndBody(std::string& requestMessage);
        void setHeaders(std::vector<std::string> linesVec, std::vector<std::string>::iterator& it);
        std::vector<std::string> splitLines(std::string str, char sep);
        void setBody(std::vector<std::string> linesVec, std::vector<std::string>::iterator itFromBody);
        std::string createBodyStringFromLinesVector(std::vector<std::string> linesVec);
        void bodyUnChunk(std::vector<std::string> linesVec, std::vector<std::string>::iterator itFromBody);
    private:

    private:
        std::string method;
        std::string path;
        std::string version;
        headersMap headers;
        std::string body;

    public:
        void test__headers();
};

#endif