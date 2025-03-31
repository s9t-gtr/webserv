#ifndef METAVARIABLES_HPP
# define METAVARIABLES_HPP

#include <string.h>
#include <sstream>
#include "../../HttpConnection/HttpConnection.hpp"

typedef void (*EnvironFunction)(progressInfo*, size_t);

class MetaVariables {
    public: 
        MetaVariables();
        ~MetaVariables();

        size_t count();
        std::map<std::string, EnvironFunction> variablesToFuncPtr;

    private:
        std::map<std::string, EnvironFunction> createVariableToFuncPtr();

        static void storeAuthType(progressInfo *obj, size_t i);
        static void storeContentLength(progressInfo *obj, size_t i);
        static void storeContentType(progressInfo *obj, size_t i);
        static void storeGatewayInterface(progressInfo *obj, size_t i);
        static void storePathInfo(progressInfo *obj, size_t i);
        static void storePathTranslated(progressInfo *obj, size_t i);
        static void storeQueryString(progressInfo *obj, size_t i);
        static void storeRemoteAddr(progressInfo *obj, size_t i);
        // static void storeRemoteHost(progressInfo *obj, size_t i);
        // static void storeRemoteIdent(progressInfo *obj, size_t i);
        // static void storeRemoteUser(progressInfo *obj, size_t i);
        static void storeRequestMethod(progressInfo *obj, size_t i);
        static void storeScriptName(progressInfo *obj, size_t i);
        static void storeServerName(progressInfo *obj, size_t i);
        static void storeServerPort(progressInfo *obj, size_t i);
        static void storeServerProtocol(progressInfo *obj, size_t i);

};

#endif
