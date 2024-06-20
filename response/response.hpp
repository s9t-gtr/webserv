#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include "../request/RequestParse.hpp"
class Response: public RequestParse{
    public:
        Response();
        ~Response();
        Response(const Response& other);
        Response& operator=(const Response& other);
    public:
        void createResponse(requestParse& request);
    private:
        //requestの値が正しいかを確認するのをparseの時にしたほうが無駄な処理が省けるが、
        void checkRequestLine();
        void checkRequestStatusCode();
        void checkRequestTarget();
        void checkRequestVersion();

        std::string createStatusline();
        std::string createHeaders()
        std::string createBody();

        //createStatusline functions
    
        
        
    private:
        std::string query;
        std::string response;
};
#endif
