#include "Response.hpp"
#include "../Request/Request.hpp"
#include "../../HttpConnection/HttpConnection.hpp"

HttpMessageParser::DetailStatus::StartLineReadStatus Response::createInitialStatus() {
    DetailStatus::StartLineReadStatus startLineStatus;
    startLineStatus.statusLineStatus = DetailStatus::StartLineReadStatus::SpaceBeforeHttpVersion;
    return startLineStatus;
}

Response::Response(): HttpMessageParser(createInitialStatus()){
    statusCode = 0;
    isReturn = false;
}

Response::~Response(){}

void Response::parseStartLine(char c){
    /*
        cがasciiであることは保証されている
    */
    switch(getDetailStatus().startLineStatus.statusLineStatus){
        case DetailStatus::StartLineReadStatus::SpaceBeforeHttpVersion:
            if(std::isspace(c)){
                if(c == CHAR_CR || c == CHAR_LF) // nginx behavior
                    return ;
                else
                    throw BadRequest_400("error");
            }
            toTheNextStatus();
            // httpVersion += c;
            break;
        case DetailStatus::StartLineReadStatus::HttpVersion:
            if(std::isspace(c)){
                if(c == ' '){ // nginx behavior
                    // if(isAllowMethod())
                        // throw NotAllowed_405("error");
                    toTheNextStatus();
                }else{
                    throw BadRequest_400("error");
                }
            }
            // method += c;
            break;
        case DetailStatus::StartLineReadStatus::SpaceBeforeStatusCode:
            if(std::isspace(c)){
                if(c == ' ') // nginx behavior
                    return ;
                else
                    throw BadRequest_400("error");
            }
            toTheNextStatus();
            // requestTarget += c;
            break;
        case DetailStatus::StartLineReadStatus::StatusCode:
            if(isspace(c)){
                if(c == ' '){
                    toTheNextStatus();
                    return ;
                }
                else if(c == CHAR_CR){
                    setIsWatingLF(true);
                    toTheNextStatus();
                    return ;
                }
                else if(c == CHAR_LF){
                    // version = "";
                    setReadingStatus(Headers);
                    setDetailStatus(DetailStatus::FirstLineForbiddenSpace);
                    return ;
                }
                else
                    throw Found_302("error"); //nginx behavior
            }
            // requestTarget += c;
            break;
        case DetailStatus::StartLineReadStatus::SpaceBeforeReasonPhrase:
            checkIsWatingLF(c, NOT_EXIST_REASONPHRASE);
            if(isspace(c)){
                if(c == ' ')
                    return;
                else if(c == CHAR_CR){
                    setIsWatingLF(true);
                    return ;
                }
                else if(c == CHAR_LF){
                    // version = "";
                    setReadingStatus(Headers);
                    setDetailStatus(DetailStatus::FirstLineForbiddenSpace);
                    return ;
                }
                else
                    throw Found_302("error");
            }
            if(c == 'H'){
                toTheNextStatus();
                // version += c;
                return ;
            }
            if(c == '%')
                throw BadRequest_400("error");
            throw Found_302("error");
            break;
        case DetailStatus::StartLineReadStatus::ReasonPhrase:
            if(std::isspace(c)){
                if(c == ' '){
                    toTheNextStatus();
                    return ;
                }else if(c == CHAR_CR){
                    setIsWatingLF(true);
                    toTheNextStatus();
                    return ;
                }else if(c == CHAR_LF){
                    setReadingStatus(Headers);
                    setDetailStatus(DetailStatus::FirstLineForbiddenSpace);
                    return ;
                }else
                    throw BadRequest_400("error");
            }
            // version += c;
            break;
        // case DetailStatus::StartLineReadStatus::TrailingSpaces:
        //     checkIsWatingLF(c, EXIST_VERSION);
        //     if(std::isspace(c)){
        //         if(c == ' '){ //RFC 9112 (2.2)
        //             return ;
        //         }else if(c == CHAR_CR){
        //             setIsWatingLF(true);
        //             return ;
        //         }else if(c == CHAR_LF){
        //             toTheNextStatus();
        //             return ;
        //         }else
        //             throw BadRequest_400("error");
        //     }
        //     throw BadRequest_400("error");
        //     break;
        default:
            break;
    }
    
}

void Response::checkIsWatingLF(char c, bool isExistThirdElement){
    if(getIsWatingLF()){
        setIsWatingLF(false);
        if(c == CHAR_LF){
            if(!isExistThirdElement) 
                statusPhrase = "";
            setReadingStatus(Headers);
            return ;
        }
        else
            throw BadRequest_400("error");
    }
}


ResponseType_t Response::createResponseFromStatusCode(StatusCode_t statusCode, Request requestInfo){
    this->statusCode = statusCode;
    std::string filePath = "documents/";
    std::string statusLine = "HTTP/1.1 ";
    const std::string mustHeader[] = {"Server", "Date", "Content-Type", "Content-Length", "Connection"};
    std::vector<std::string> headers(mustHeader, mustHeader + 5);
    switch(statusCode){
        case 200:
            statusLine += "200 0K\r\n";
            filePath = requestInfo.getPath();
            break;
        case 302:
            statusLine += "302 Moved Temporarily\r\n";
            filePath += "302.html";
            headers.push_back("Location");
            break;
        case 400:
            statusLine += "400 Bad Request\r\n";
            filePath += "400.html";
            break;
        case 403:
            statusLine += "403 Forbidden\r\n";
            filePath += "403.html";
            break;
        case 404:
            statusLine += "404 Not Found\r\n";
            filePath += "404.html";
            break;
        case 405:
            statusLine += "405 Not Allowed\r\n";
            filePath += "405.html";
            break;
        case 413:
            statusLine += "413 Request Entity Too Large\r\n";
            filePath += "413.html";
            break;
        case 500:
            statusLine += "500 Internal Server Error\r\n";
            filePath += "500.html";
            break;
        case 501:
            statusLine += "501 Not Implemented\r\n";
            filePath += "501.html";
            break;
        case 504:
            statusLine += "504 Gateway Timeout\r\n";
            filePath += "504.html";
            break;

    }
    response += statusLine;
    std::string html;
    if(requestInfo.getIsAutoindex())
        html = createAutoindexPage(requestInfo);
    else
        html = getHtml(filePath);
    response += createHeaders(headers, html.size(), requestInfo) + "\r\n";
    response += html;
    return STATIC_PAGE;
}


std::string Response::getHeader(std::string fieldName, std::string::size_type content_length, Request requestInfo){
    if(fieldName == "Server")
        return "Server: webserv/1.0\r\n";
    else if(fieldName == "Date")
        return "Date: " + getGmtDate() + "\r\n";
    else if(fieldName == "Content-Length")
        return "Content-Length: " + std::to_string(content_length) + "\r\n";
    else if(fieldName == "Connection")
        return "Connection: Keep-Alive\r\n";
    else if(fieldName == "Content-Type")
        return "Content-Type: text/html\r\n";
    else if(fieldName == "Location"){
        if(isReturn)
            return "Location: " + requestInfo.getLocation()->locationSetting["return"] + "\r\n";
        else
            return "Location: " + requestInfo.getServer()->serverSetting["error_page"] + "\r\n";
    }
    return "";
}

std::string Response::createHeaders(std::vector<std::string> needHeaders, std::string::size_type content_length, Request requestInfo){
    std::string headers;
    std::vector<std::string>::iterator end = needHeaders.end();
    for(std::vector<std::string>::iterator fieldName = needHeaders.begin(); fieldName != end; fieldName++){
        headers += getHeader(*fieldName, content_length, requestInfo);
    }
    return headers;
}

ResponseType_t Response::createResponse(progressInfo *obj){
    statusCode = obj->requestInfo.confirmRequest();
    if(statusCode){ //already set 
        return createResponseFromStatusCode(statusCode, obj->requestInfo);
    }
    if (obj->requestInfo.getLocation()->locationSetting["return"] != "none"){
        isReturn = true;
        return createResponseFromStatusCode(302, obj->requestInfo);
    }
    if(obj->requestInfo.getMethod() == "GET"){
        if(obj->requestInfo.getIsCgi()){
            return createResponseByCgi(obj);
        }
        return createResponseFromStatusCode(200, obj->requestInfo);
    }else if(obj->requestInfo.getMethod() == "POST" || obj->requestInfo.getMethod() == "DELETE"){
        return createResponseByCgi(obj);
    }else if(obj->requestInfo.getMethod() == "DELETE"){
        /*
            nginxで動作確認したところ、ファイルとそれが入っているディレクトリの権限を持たせた上でlimit_exceptディレクティブを使用してもDELETEメソッドのリクエストには405 NOT ALLOWEDが返った。
            調べてみるとnginxは静的ファイルに対するDELETEメソッドをサポートしていないので、バックエンドで削除する処理を実行する必要があるらしい:(https://stackoverflow.com/questions/71828435/nginx-delete-method-not-allowed)
                ・・・ngx_http_dav_moduleというモジュールをインストールする方法もあるらしい(試してない)
            CGI R
            なお、Respons status codeはCGIスクリプトの実装に依存することになると考えられるため、ここでのresponse status codeは課題要件の範囲外となる。HTTPの常識の範囲内で返すことにする。
        */
        if (std::remove(obj->requestInfo.getPath().c_str()) != 0)// ファイルを削除
            perror("remove error");
    }
    return STATIC_PAGE;
}

ResponseType_t Response::createResponseByCgi(progressInfo *obj){
    if(pipe(pipe_c2p) < 0)
        throw std::runtime_error("Error: pipe() failed");
    pid_t pid = fork();
    if(pid == 0){
        executeCgi(obj->requestInfo);
    } else if(pid > 0) {
        pidfd = pid;
        return CGI;
    } else { //fork失敗
        // sendInternalErrorPage(obj);
        close(pipe_c2p[R]);
        close(pipe_c2p[W]);
        return createResponseFromStatusCode(500, obj->requestInfo);
    }
    return CGI;
}


#include <fcntl.h>
void Response::executeCgi(Request& requestInfo){
    close(pipe_c2p[R]);
    dup2(pipe_c2p[W],1);
    close(pipe_c2p[W]);
    extern char** environ;
    std::string path = requestInfo.getPath();
    const char* cPath = path.c_str();
    std::cout << "cPath: " << cPath << std::endl;
    char* const cgi_argv[] = { const_cast<char*>(cPath), NULL };
    if(execve(cPath, cgi_argv, environ) < 0){
        exit(1);
    }
}


/*
    utils
*/



std::string Response::getGmtDate()
{
    std::time_t now = std::time(nullptr);
    std::tm* tm_now = std::gmtime(&now);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm_now);
    
    return (std::string(buffer));
}

std::string Response::getHtml(std::string filePath){
    std::ifstream file(filePath);
    if (!file.is_open()) //https://docs.oracle.com/cd/E19205-01/820-2985/loc_io/9_2.htm
        perror("open error");
    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
        content += "\n";
    }
    file.close();
    return content;
}

/*
    getter and setter
*/

std::string Response::getResponse(){
    return response;
}
void Response::setResponse(std::string setResponse){
    response = setResponse;
}
StatusCode_t Response::getStatusCode(){
    return statusCode;
}
void Response::setStatusCode(StatusCode_t setStatusCode){
    statusCode = setStatusCode;
}
pid_t Response::getPidfd(){
    return pidfd;
}
int Response::getPipefd(int RorW){
    if(RorW == R)
        return pipe_c2p[R];
    if(RorW == W)
        return pipe_c2p[W];
    return 0;
}

void Response::clean() {
    statusCode = 0;
    response = "";
    isReturn = false;
}