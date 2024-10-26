#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include <fstream>
#include <set>
#include "../HttpMessageParser.hpp"
#include "../Request/Request.hpp"
// #include "../../HttpConnection/HttpConnection.hpp"

typedef unsigned int ResponseType_t;
typedef struct progressInfo progressInfo;

#define NOT_EXIST_REASONPHRASE false
#define R 0
#define W 1

#define STATIC_PAGE 0
#define CGI 1


class Response: public HttpMessageParser{
    public:
        HttpMessageParser::DetailStatus::StartLineReadStatus createInitialStatus();
        Response();
        virtual ~Response();
        ResponseType_t createResponse(progressInfo *obj);
        ResponseType_t createResponseFromStatusCode(StatusCode_t statusCode, Request requestInfo);

        std::string getResponse();
        void setResponse(std::string setResponse);
        StatusCode_t getStatusCode();
        void setStatusCode(StatusCode_t setStatusCode);
        pid_t getPidfd();
        pid_t getPipefd(int RorW);
        
    private:
        StatusCode_t statusCode;
        std::string response;

        int pipe_c2p[2];
        int pidfd;

        bool isReturn;
        //cgiレスポンス受け取り用 (headersとbodyはHttpMessageParserで定義されている)
        virtual void parseStartLine(char c);
        virtual void checkIsWatingLF(char c, bool isExistThirdElement);
        std::string statusPhrase;

        std::string createAutoindexPage(Request& requestInfo);
        std::string createHeaders(std::vector<std::string> needHeaders, std::string::size_type content_length, Request requestInfo);
        std::string getHeader(std::string fieldName, std::string::size_type content_length, Request requestInfo);

        ResponseType_t createResponseByCgi(progressInfo *obj);
        void executeCgi(Request& requestInfo);

        //utils
        std::string getGmtDate();
        std::string getHtml(std::string filePath);
};

#endif