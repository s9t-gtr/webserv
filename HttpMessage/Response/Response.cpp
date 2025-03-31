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

bool determineResponseTypeByHttpVersion(std::string tmpHttpVersion);
void Response::parseStartLine(char c){
    /*
        cがasciiであることは保証されている

        CGI Responseが不正の時は502を返すが、不正の条件はRequestの時と異なることに注意
            HTTP Resposneとして不完全なCGI Responseをnginxが補完して正しく返すケースがある
             - [ header, body ] -> status lineを補完？ brawserが解釈？
             - [ startline, body ] -> serverがヘッダ補完
             - contentlengthの有無
                - 有 Content-Length: (x >= 1) && bodyなし -> 1分待機後fail(response statusCodeなしのエラー扱い)
                - 有 Content-Length: (x < bodyLength) && bodyあり -> 設定した文字数分はbodyを送信できるが、15秒ほどserver内部で続きのCGI Respnoseを待つ
                - 無 && bodyあり -> 全てclientに送信される
                - 
            - [ body ] -> 502
		
    */
    if(!isascii(c)){
        syntaxErrorDetected();
    }
    switch(getDetailStatus().startLineStatus.statusLineStatus){
        case DetailStatus::StartLineReadStatus::SpaceBeforeHttpVersion:
            if(std::isspace(c)){
                if(c == CHAR_CR || c == CHAR_LF) // nginx behavior
                    return ;
                else
                    throw BadCgiResponse("error");
            }
            toTheNextStatus();
            httpVersion += c;
            break;
        case DetailStatus::StartLineReadStatus::HttpVersion:
            if(std::isspace(c)){
                if(c == ' '){ // nginx behavior
                    /*
                        この空白はHeader Fieldの区切り文字かもしれないため、httpVersionの要件を満たすか確認する
                        httpVersionではなかった場合、status lineなしのCGI Responseだと判断できるため、これまでのものは破棄してReadingStatusをHeadersとしてパースし直す
                    */
                    if(determineResponseTypeByHttpVersion(httpVersion)) {
                        toTheNextStatus();
                        return ;
                    }
                    httpVersion = "";
                    setReadingStatus(Headers);
                    setDetailStatus(DetailStatus::FirstLineForbiddenSpace);
                    return ;
                }
                else if(c == CHAR_CR) {
                    /*
                        空白が来てtoTheNextStatusが来る前にCRが来た場合、status lineなしのCGI Responseだと判断できる
                            - "Content-Length:5\r\n"などheader fieldの空白がないこともあるため
                        LFまで読み込んでhttpVersionだったものにparseHeader()を通すReadingStatusをHeadersとしてパースし直す
                    */
                    httpVersion = "";
                    setReadingStatus(Headers);
                    setDetailStatus(DetailStatus::FirstLineForbiddenSpace);
                    return ;
                }
                else{
                    throw BadCgiResponse("error");
                }
            }
            httpVersion += c;
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
                    toTheNextStatus(); //NOTICE: どうなる？
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
            throw BadGateway_502("error");
    }
}

bool determineResponseTypeByHttpVersion(std::string tmpHttpVersion) {
    /*
	ClientへのResponseとして、HTTP Responseを返すのかHTMLファイルを返すかを決定する関数
     */
    if(!tmpHttpVersion.size()) { //throw std::runtime_error("先頭の空白を事前に削除できていない"); }
	return false;
    }
    if(tmpHttpVersion[0] == 'H') { return true; } //一文字目がHならHTTP Responseを返すことができる
						  //
    if(tmpHttpVersion.size() >= 5) { return false; }
    if(tmpHttpVersion.substr(0, 5) != "HTTP/") { return false; }
    if(tmpHttpVersion.size() < 7) { return false; }
    std::string version = tmpHttpVersion.substr(6);
    std::string::size_type commaIndex = version.find(".");
    if( commaIndex == std::string::npos ) { return false; }
    //NOTICE: .の存在は確認したが数値チェック省略
    // std::vector<std::string> splitByComma = split(version, '.');
    // if (splitByComma.size() != 2) { return false; }
    // for(std::vector<std::string>::iterator it=splitByComma.begin(); it!=splitByComma.end(); it++){
    //     std::ostringstream oss;
    //     oss << *it;
    //     /*
    //         詳しいversion数値チェックの挙動不明
    //     */
    // }
    return true;
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
        case 301:
            statusLine += "301 Moved Permanently\r\n";
            filePath += "301.html";
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
            filePath = requestInfo.getServer()->serverSetting["error_page"];
            break;
        case 405:
            statusLine += "405 Not Allowed\r\n";
            filePath += "405.html";
            break;
        case 413:
            statusLine += "413 Request Entity Too Large\r\n";
            filePath += "413.html";
            break;
		case 414:
			statusLine += "414 Request-URI Too Long\r\n";
			filePath += "414.html";
			break;
        case 500:
            statusLine += "500 Internal Server Error\r\n";
            filePath += "500.html";
            break;
        case 501:
            statusLine += "501 Not Implemented\r\n";
            filePath += "501.html";
            break;
        case 502:
            statusLine += "502 Bad Gateway\r\n";
            filePath += "502.html";
            break;
        case 504:
            statusLine += "504 Gateway Timeout\r\n";
            filePath += "504.html";
            break;
		case 505:
			statusLine += "505 HTTP Version Not Supported\r\n";
			filePath += "505.html";
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
    
    if (requestInfo.getRequestTarget() != "" && !determineResponseTypeByHttpVersion(requestInfo.getVersion())) {
	response = html; // TODO: 確かにここでresponseをhtmlだけにする処理は欲しいけど、Versionの頭文字が'H'なら400 http responseを返さないといけないので、ここにくるまでにVersionを見てreturn 400とする関数がRequest.cppに必要
    }
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
	std::cerr << "content-type" << obj->requestInfo.getField("content-type") << std::endl;
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
    if(pipe(pipe_p2c) < 0)
        throw std::runtime_error("Error: pipe() failed");
    if(pipe(pipe_c2p) < 0)
        throw std::runtime_error("Error: pipe() failed");
    pid_t pid = fork();
    if(pid == 0){
        executeCgi(obj);
    } else if(pid > 0) {
		pidfd = pid;
		close(pipe_p2c[R]); // 子の読み取り側を閉じる
   		ssize_t written_byte = write(pipe_p2c[W], obj->requestInfo.getBody().c_str(), obj->requestInfo.getContentLength()); // POSTデータを書き込む
   		close(pipe_p2c[W]); // データを書き終わったら閉じる
		std::cerr << "written_byte: " << written_byte << std::endl;
        return CGI;
    } else { //fork失敗
        // sendInternalErrorPage(obj);
        close(pipe_c2p[R]);
        close(pipe_c2p[W]);
        close(pipe_p2c[R]);
        close(pipe_p2c[W]);
        return createResponseFromStatusCode(500, obj->requestInfo);
    }
    return CGI;
}


void Response::executeCgi(progressInfo *obj){
	close(pipe_p2c[W]);
	dup2(pipe_p2c[R], 0);
	close(pipe_p2c[R]);

    close(pipe_c2p[R]);
    dup2(pipe_c2p[W],1);
    close(pipe_c2p[W]);
    Request& requestInfo = obj->requestInfo;
    std::string script = requestInfo.getPath();
    if(script.size() > 3 && script.substr(0, 2) == "./") {
        script = script.substr(2);
    }
    extern char **environ;
    createEnviron(obj, environ);
    // std::cerr << "=== Environment Variables Before execve ===\n";
    // for (char **env = obj->holdMetaVariableEnviron; *env; ++env) {
    //     std::cerr << *env << std::endl;
    // }
    // std::cerr << "===========================================\n";
    // std::cerr << script << std::endl;
    if(script.substr(script.length() - 3) == ".py"){
		const char* script_c = script.c_str();
        char* const cgi_argv[] = { NULL };
        if(execve(script_c, cgi_argv, obj->holdMetaVariableEnviron) < 0){
			perror("execve");
            exit(1);
		}
    }
    const char* cPath = script.c_str();
    std::cout << "cPath: " << cPath << std::endl;
    char* const cgi_argv[] = { const_cast<char*>(cPath), NULL };
    if(execve(cPath, cgi_argv, obj->holdMetaVariableEnviron) < 0){
        exit(1);
    }
}



std::vector<std::string> splitBody(std::string httpMessage) {
    std::vector<std::string> result;
    std::string::size_type pos = httpMessage.find("\r\n\r\n");
    if (pos) {
        result.push_back(httpMessage.substr(0, pos));
        result.push_back(httpMessage.substr(pos+1));
        return result;
    }
    pos = httpMessage.find("\n\n");
    if (pos) {
        result.push_back(httpMessage.substr(0, pos));
        result.push_back(httpMessage.substr(pos+1));
        return result;
    }
    result.push_back(httpMessage);
    return result;
}

std::vector<std::string> getLines(std::string startLineOrHeaders) {
    /*
        startLineOrHeaders means [ startLine || Headers ]
            contain only startline, or only Headers 
    */
    std::string::size_type size = startLineOrHeaders.size();
    std::vector<std::string> result;
    for (std::string::size_type i=0; i<size; i++){
        std::string::size_type pos = startLineOrHeaders.find(CRLF, i);
        if (pos) {
            result.push_back(startLineOrHeaders.substr(i, pos));
            i = pos + string(CRLF).size();
            continue;
        }
        pos = startLineOrHeaders.find(STR_LF, i);
        if (pos) {
            result.push_back(startLineOrHeaders.substr(i, pos));
            i = pos + string(STR_LF).size();
        }
    }
    return result;
}

bool isStatusLine(std::string firstLine) {
    /*
        case of return true, firstLine have 3 elements
            first: 
                HTTP version
            second:
                status code 
            third:
                reason phrase
    */
	(void)firstLine;
    return false;
}


std::vector<HttpMessageParser::ReadingStatus> Response::getCgiResponseComponents(std::string cgiResponse){
    std::vector<ReadingStatus> result;
    std::string::size_type responseSize = cgiResponse.size();

    if (!responseSize) { return result; }
    
    std::vector<std::string> splitBodyResponse = splitBody(cgiResponse);


    std::vector<std::string> linesOfStartLineOrHeaders = getLines(splitBodyResponse[0]);
    if (isStatusLine(linesOfStartLineOrHeaders[0])) {
        result.push_back(StartLine);
    }
    bool isHasStatusLine = std::find(result.begin(), result.end(), StartLine) != result.end();
    if (isHasStatusLine && linesOfStartLineOrHeaders.size() > 1 ) {
        result.push_back(Headers);
    } else if (!isHasStatusLine && linesOfStartLineOrHeaders.size() > 0) {
        result.push_back(Headers);
    }
    if(splitBodyResponse.size() > 1) {
        result.push_back(Body);
    }

    return result;
    
    
}

bool Response::checkCgiResponseSyntax(std::string cgiResponse) {
    /*
        return false条件
        - header fieldが1行もない
    */

   	(void)cgiResponse; 
    return true;
}



// void store(progressInfo *obj, size_t i) {

// }

#include "MetaVariables.hpp"


void Response::createEnviron(progressInfo *obj, char **environ){
    MetaVariables metaVariables;

    size_t i = 0;
    numEnvironLine = 0;
    while(environ[i++] != NULL){
        numEnvironLine++;
    }
    obj->holdMetaVariableEnviron = (char **)std::malloc((numEnvironLine + metaVariables.count() + 1) * sizeof(char *));
    i = 0;
    while(i < numEnvironLine){
        obj->holdMetaVariableEnviron[i] = strdup(environ[i]);
        i++;
    }
    for (std::map<std::string, EnvironFunction>::iterator it = metaVariables.variablesToFuncPtr.begin(); it!=metaVariables.variablesToFuncPtr.end(); it++){
        it->second(obj, i++);
    }
    obj->holdMetaVariableEnviron[i] = NULL;
    
}
// void Response::createEnviron(progressInfo *obj, char **environ){
//     size_t numEnvironLine = 0;
//     while (environ[numEnvironLine] != NULL) {
//         numEnvironLine++;
//     }

//     // HOGE=hoge を追加するため +2 (HOGE + NULL)
//     obj->holdMetaVariableEnviron = (char **)malloc((numEnvironLine + 2) * sizeof(char *));
//     if (!obj->holdMetaVariableEnviron) {
//         perror("malloc failed");
//         exit(1);
//     }

//     // 既存の環境変数をコピー
//     size_t i = 0;
//     for (; i < numEnvironLine; i++) {
//         obj->holdMetaVariableEnviron[i] = strdup(environ[i]);
//         if (!obj->holdMetaVariableEnviron[i]) {
//             perror("strdup failed");
//             exit(1);
//         }
//     }

//     // HOGE=hoge を追加
//     obj->holdMetaVariableEnviron[i] = strdup("HOGE=hoge");
//     if (!obj->holdMetaVariableEnviron[i]) {
//         perror("strdup failed");
//         exit(1);
//     }

//     // NULL 終端
//     obj->holdMetaVariableEnviron[i + 1] = NULL;
// }



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
