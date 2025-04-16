#ifndef METAVARIABLES_HPP
# define METAVARIABLES_HPP

#include <string.h>
#include <sstream>
#include "HttpConnection.hpp"

struct progressInfo;
typedef void (*EnvironFunction)(progressInfo *obj, RequestParse &requestInfo, size_t);

class MetaVariables {
    public: 
        MetaVariables();
        ~MetaVariables();

        size_t count();
        std::map<std::string, EnvironFunction> variablesToFuncPtr;

    private:
        std::map<std::string, EnvironFunction> createVariableToFuncPtr();

        static void storeAuthType(progressInfo *obj, RequestParse &requestInfo, size_t i);
        // static void storeContentLength(progressInfo *obj, RequestParse &requestInfo, size_t i);
        static void storeContentType(progressInfo *obj, RequestParse &requestInfo, size_t i);
        // static void storeGatewayInterface(progressInfo *obj, RequestParse &requestInfo, size_t i);
        static void storePathInfo(progressInfo *obj, RequestParse &requestInfo, size_t i);
        static void storePathTranslated(progressInfo *obj, RequestParse &requestInfo, size_t i);
        static void storeQueryString(progressInfo *obj, RequestParse &requestInfo, size_t i);
        // static void storeRemoteAddr(progressInfo *obj, RequestParse &requestInfo, size_t i);
        // static void storeRemoteHost(progressInfo *obj, RequestParse &requestInfo, size_t i);
        // static void storeRemoteIdent(progressInfo *obj, RequestParse &requestInfo, size_t i);
        // static void storeRemoteUser(progressInfo *obj, RequestParse &requestInfo, size_t i);
        static void storeRequestMethod(progressInfo *obj, RequestParse &requestInfo, size_t i);
        static void storeScriptName(progressInfo *obj, RequestParse &requestInfo, size_t i);
        static void storeServerName(progressInfo *obj, RequestParse &requestInfo, size_t i);
        static void storeServerPort(progressInfo *obj, RequestParse &requestInfo, size_t i);
        // static void storeServerProtocol(progressInfo *obj, RequestParse &requestInfo, size_t i);

};

#endif
