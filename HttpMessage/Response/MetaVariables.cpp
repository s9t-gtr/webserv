#include "MetaVariables.hpp"

MetaVariables::MetaVariables() {
    variablesToFuncPtr = createVariableToFuncPtr();
}

std::map<std::string, EnvironFunction> MetaVariables::createVariableToFuncPtr() {
    std::map<std::string, EnvironFunction> result;
    result["AUTH_TYPE"] = storeAuthType;
    result["CONTENT_LENGTH"] = storeContentLength;
    result["REQUEST_METHOD"] = storeRequestMethod;
    result["QUERY_STRING"] = storeQueryString;
    result["CONTENT_TYPE"] = storeContentType;
    // result[""] = ;
    // result[""] = ;
    return result;
}

MetaVariables::~MetaVariables() {}


size_t MetaVariables::count() {
    return variablesToFuncPtr.size();
}
/*
    functions of store meta variable ()
*/
void MetaVariables::storeAuthType(progressInfo *obj, size_t i) {
    std::string auth_type = split(obj->requestInfo.getField("authorization"), ' ')[0];
    obj->holdMetaVariableEnviron[i] = (char *)("AUTH_TYPE=" + auth_type).c_str();
}

void MetaVariables::storeContentLength(progressInfo *obj, size_t i) {
    std::string::size_type content_length = obj->requestInfo.getContentLength();
    std::ostringstream oss;
    oss << content_length;
    std::string content_length_str = oss.str();
    obj->holdMetaVariableEnviron[i] = strdup(("CONTENT_LENGTH=" + content_length_str).c_str());

}

void MetaVariables::storeContentType(progressInfo *obj, size_t i) {
    obj->holdMetaVariableEnviron[i] = strdup(("CONTENT_TYPE=" + obj->requestInfo.getField("content-type")).c_str());
}


void MetaVariables::storeGatewayInterface(progressInfo *obj, size_t i) {
    std::string gateway = "CGI/1.1";
    obj->holdMetaVariableEnviron[i] = strdup(("GATEWAY_INTERFACE=" + gateway).c_str());
}

std::string getPathInfo(std::string path);
void MetaVariables::storePathInfo(progressInfo *obj, size_t i) {
    std::string pathInfo = getPathInfo(obj->requestInfo.getPath());
    obj->holdMetaVariableEnviron[i] = strdup(("PATH_INFO=" + pathInfo).c_str());
}
std::string getPathInfo(std::string path) {
    /*
        このままだとQUERYSTRINGも含まれるので注意
    */
    bool afterScriptPath = false;
    std::string pathInfo = "";
    std::vector<std::string> splitPath = split(path, '/');
    for(std::vector<std::string>::iterator it = splitPath.begin(); it != splitPath.end(); it++){
        size_t pathLength = (*it).size();
        if(!afterScriptPath && (((*it).substr(pathLength > 4 ? pathLength - 4 : 0) == ".cgi") || ((*it).substr(pathLength > 2 ? pathLength - 2 : 0) == ".py"))) {
            afterScriptPath = true;
            continue;
        }
        if(afterScriptPath) {
            pathInfo += "/" + *it;
        }
    }
    return pathInfo;
}

void MetaVariables::storePathTranslated(progressInfo *obj, size_t i) {
    /*
        PATH_INFOがnullならPATH_TRANSLATEDもnull <- RFC3875(4.1.6.)
    */
    std::string pathTranslated = getPathInfo(obj->requestInfo.getPath()) == "" ? "" : "";
    obj->holdMetaVariableEnviron[i] = strdup(("PATH_TRANSLATED=" + pathTranslated).c_str());
}

std::string getDecodedQueryString(std::string queryString);
void MetaVariables::storeQueryString(progressInfo *obj, size_t i) {
	if (obj->requestInfo.getQueryString() == "") {
		obj->holdMetaVariableEnviron[i] = strdup("QUERY_STRING=");
		return;
	}
	std::string decodedQueryString = getDecodedQueryString(obj->requestInfo.getQueryString());
    obj->holdMetaVariableEnviron[i] = strdup(("QUERY_STRING=" + obj->requestInfo.getQueryString()).c_str());
}
std::string getDecodedQueryString(std::string encodedQueryString) {
	std::string decodedQueryString = "";
	size_t encodedQueryStringLength = encodedQueryString.size();
	for (size_t i = 0; i < encodedQueryStringLength; i++) {
		if (encodedQueryString[i] == '%' && i + 2 < encodedQueryStringLength) {
			int hexValue;
			std::istringstream hexStream(encodedQueryString.substr(i + 1, 2));
			hexStream >> std::hex >> hexValue;
			decodedQueryString += static_cast<char>(hexValue);
			i += 2;
		} else if (encodedQueryString[i] == '+') {
			decodedQueryString += ' ';
		}
		else {
			decodedQueryString += encodedQueryString[i];
		}
	}
	return decodedQueryString;
}

void MetaVariables::storeRemoteAddr(progressInfo *obj, size_t i) {
    obj->holdMetaVariableEnviron[i] = strdup(("REMOTE_ADDR=" + obj->clientIpAddress).c_str());
}

// void MetaVariables::storeRemoteHost(progressInfo *obj, size_t i) {
//     /*
//         getnameinfo()を使用しないと取得不可能だと思ったが、requestのhostヘッダに書いてあった
//         かつmeta_variable_check.pyでわかるようにnginxでREMOTE_HOSTは読み込まれていないので不要と判断
//     */
//     obj->holdMetaVariableEnviron[i] = strdup(("REMOTE_HOST: " + obj->clientHostname).c_str());
// }

// void MetaVariables::storeRemoteIdent(progressInfo *obj, size_t i) {
//     /*
//         meta_variable_check.pyでわかるようにnginxでREMOTE_IDENTは読み込まれていないので不要と判断
//     */
// }
// void MetaVariables::storeRemoteUser(progressInfo *obj, size_t i) {
//     /*
//         meta_variable_check.pyでわかるようにnginxでREMOTE_USERは読み込まれていないので不要と判断
//     */
// }
std::string getScriptName(const std::string locationPath, const std::string requestPath) ;
void MetaVariables::storeScriptName(progressInfo *obj, size_t i) {
    /*
        configのlocationにmatchした部分からのpathになる
        location /cgi_bin/ { ... }の時は /cgi_bin/script.cgi 
    */
    std::string script_name = getScriptName(obj->requestInfo.getLocation()->path, obj->requestInfo.getPath());
    obj->holdMetaVariableEnviron[i] = strdup(("SCRIPT_NAME=" + script_name).c_str());
}
std::string getScriptName(const std::string locationPath, const std::string requestPath) {
    return requestPath.substr(requestPath.find(locationPath, 0));
}

void MetaVariables::storeServerName(progressInfo *obj, size_t i) {
    obj->holdMetaVariableEnviron[i] = strdup(("SERVER_NAME=" + obj->requestInfo.getServer()->getServerName()).c_str());
}
void MetaVariables::storeServerPort(progressInfo *obj, size_t i) {
    obj->holdMetaVariableEnviron[i] = strdup(("SERVER_PORT=" + obj->requestInfo.getServer()->getListenPort()).c_str());
}
void MetaVariables::storeServerProtocol(progressInfo *obj, size_t i) {
    std::string protocol = "HTTP/1.1";
    obj->holdMetaVariableEnviron[i] = strdup(("SERVER_PORT=" + protocol).c_str());
}

void MetaVariables::storeRequestMethod(progressInfo *obj, size_t i) {
    obj->holdMetaVariableEnviron[i] = strdup(("REQUEST_METHOD=" + obj->requestInfo.getMethod()).c_str());
}


