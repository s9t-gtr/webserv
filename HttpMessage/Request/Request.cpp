#include "Request.hpp"

HttpMessageParser::DetailStatus::StartLineReadStatus Request::createInitialStatus() {
    setReadingStatus(StartLine);

    DetailStatus::StartLineReadStatus::RequestLineReadStatus requestLineReadStatus;
    requestLineReadStatus = DetailStatus::StartLineReadStatus::SpaceBeforeMethod;
    setDetailStatus(requestLineReadStatus);

    DetailStatus::StartLineReadStatus startLineStatus;
    startLineStatus.requestLineStatus = DetailStatus::StartLineReadStatus::SpaceBeforeMethod;
    return startLineStatus;
}

Request::Request(Config *config): HttpMessageParser(createInitialStatus()){
    this->config = config;
    isAutoindex = false;
    // bodySize = 0;
}
// Request::Request(string requestMessage, Config *conf): allocFlag(false){
//     parseRequesetLine(requestMessage);
//     setHeadersAndBody(requestMessage);
//     if (headers.find("host") == headers.end()) 
//         throw runtime_error("Error: Missing required 'host' header");
//     if (version != "HTTP/1.1")
//         throw runtime_error("Error: Invalid version");
//     setCorrespondServer(conf);
//     setCorrespondLocation();
// }

Request::~Request(){}



void convertCarriageReturnToSpace(string &line){
    string::size_type i = 0;
    while(string::size_type carriageReturnIndex = line.find(STR_CR, i) != string::npos){
        line[carriageReturnIndex] = ' ';
    }
}


// void Request::readBufferAndParse(char *buffer, size_t bufferSize){
//     readingBuffer += string(buffer, buffer + bufferSize);
//     while(string::size_type index = readingBuffer.find(CRLF) != string::npos){
//         if(readingStatus == RequestLine){
//             string requestLine = readingBuffer.substr(0, index + 1);
//             parseRequestLine(requestLine);
//             readingStatus = Headers;
//         }else if(readingStatus == Headers){
//             string requestHeader = readingBuffer.substr(0, index + 1);
//             if(requestHeader == CRLF){
//                 if(getField("content-length") != "0"){// メッセージが期待されているか
//                     readingStatus = Body;
//                     readingBuffer = readingBuffer.substr(index + 1);
//                     continue;
//                 }
//                 return ;
//             }
//             parseHeaders(requestHeader);
//         }else if(readingStatus == Body || readingStatus == Complete){
//             break;
//         }
//     }
//     if(readingStatus == Body){
//         checkOnlyAsciiCharacter(readingBuffer);
//         parseBody(readingBuffer);
//     }
// }

// string Request::getRequestLine(string& requestMessage){
//     string::size_type idx = requestMessage.find("\n", 0);
//     if(idx == string::npos)
//         throw runtime_error("Error: invalid request: non new line");
//     string firstRow = requestMessage.substr(0, idx);
//     firstRow.pop_back();
//     requestMessage = requestMessage.substr(idx+1);
//     return firstRow;
// }

// vector<string> splitByVariousSpace(string mixedSpaceString){
//     vector<string> answer;
//     string::size_type i = 0;
//     bool isReading = false;
//     string nonSpaceString = "";
//     while(i < mixedSpaceString.size()){
//         if(!isspace(mixedSpaceString[i])){
//             if(isReading){
//                 nonSpaceString += mixedSpaceString[i];
//             }else{
//                 nonSpaceString = mixedSpaceString[i];
//                 isReading = true;
//             }
//         }else{
//             if(isReading){
//                 isReading = false;
//                 if(nonSpaceString != ""){
//                     answer.push_back(nonSpaceString);
//                     nonSpaceString = "";
//                 }
//             }
//         }
//         i++;
//     }
//     return answer;
// }

void Request::parseStartLine(char c){
    /*
        cがasciiであることは保証されている
    */
    switch(getDetailStatus().startLineStatus.requestLineStatus){
        case DetailStatus::StartLineReadStatus::SpaceBeforeMethod:
            if(std::isspace(c)){
                if(c == CHAR_CR || c == CHAR_LF) // nginx behavior
                    return ;
                else
                    syntaxErrorDetected();
            }
            toTheNextStatus();
            method += c;
            break;
        case DetailStatus::StartLineReadStatus::Method:
            if(std::isspace(c)){
                if(c != ' ') // nginx behavior
                    syntaxErrorDetected();
                toTheNextStatus();
                return;

            }
            if(std::isalpha(c) && std::isupper(c))
                method += c;
            else
                syntaxErrorDetected();
            break;
        case DetailStatus::StartLineReadStatus::SpaceBeforeRequestTarget:
            if(std::isspace(c)){
                if(c == ' ') // nginx behavior
                    return ;
                else
                    syntaxErrorDetected();
            }
            toTheNextStatus();
            requestTarget += c;
            break;
        case DetailStatus::StartLineReadStatus::RequestTarget:
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
                    version = "";
                    setReadingStatus(Headers);
                    setDetailStatus(DetailStatus::FirstLineForbiddenSpace);
                    return ;
                }
                else
                    syntaxErrorDetected(302); //nginx behavior
            }
            requestTarget += c;
            break;
        case DetailStatus::StartLineReadStatus::SpaceBeforeVersion:
            checkIsWatingLF(c, NOT_EXIST_VERSION);
            if(isspace(c)){
                if(c == ' ')
                    return;
                else if(c == CHAR_CR){
                    setIsWatingLF(true);
                    return ;
                }
                else if(c == CHAR_LF){
                    version = "";
                    setReadingStatus(Headers);
                    setDetailStatus(DetailStatus::FirstLineForbiddenSpace);
                    return ;
                }
                else
                    syntaxErrorDetected(302);
            }
            if(c == 'H'){
                toTheNextStatus();
                version += c;
                return ;
            }
            if(c == '%')
                syntaxErrorDetected();
            syntaxErrorDetected(302);
            break;
        case DetailStatus::StartLineReadStatus::Version:
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
                    syntaxErrorDetected();
            }
            version += c;
            break;
        case DetailStatus::StartLineReadStatus::TrailingSpaces:
            checkIsWatingLF(c, EXIST_VERSION);
            if(std::isspace(c)){
                if(c == ' '){ //RFC 9112 (2.2)
                    return ;
                }else if(c == CHAR_CR){
                    setIsWatingLF(true);
                    return ;
                }else if(c == CHAR_LF){
                    toTheNextStatus();
                    return ;
                }else
                    syntaxErrorDetected();
            }
            syntaxErrorDetected();
            break;
        default:
            break;
    }
    
}

void Request::checkIsWatingLF(char c, bool isExistThirdElement){
    if(getIsWatingLF()){
        setIsWatingLF(false);
        if(c == CHAR_LF){
            if(!isExistThirdElement) 
                version = "";
            setReadingStatus(Headers);
            return ;
        }
        else
            syntaxErrorDetected();
    }
}


void Request::setConfigInfo(){
    setCorrespondServer();
    setCorrespondLocation();
    createPathFromConfRoot();
}

StatusCode_t Request::confirmRequest(){
    setConfigInfo();
    if(getSyntaxErrorDetected()){
        if(getResponseStatusCode() == 302)
            return 302;
        else
            return 400;
    }
    if(!isAllowMethod()){
        if(version == "")// nginx behavior: 通常versionがない時は302でhtmlファイルのみ返すが、!isAllowMethod()も重なると400になる。
            return 400;
        return 405;
    }

    return validateRequestTarget();
}

bool Request::isCgiRequest()
{
    std::string::size_type pathLength = pathFromConfRoot.size();

    if(pathFromConfRoot.find(".") == std::string::npos)
        return false;
    if(pathFromConfRoot.substr(pathLength > 4 ? pathLength - 4 : 0) == ".cgi")
        return true;
    if(pathFromConfRoot.substr(pathLength > 2 ? pathLength - 2 : 0) == ".py")
        return true;

    return false;
}

StatusCode_t Request::validateRequestTarget(){
    /*
        request targetが存在することを保証するための関数。
        流れ:
            2. pathFromConfRootが指すファイル(orディレクトリ)が存在するか
            3. ディレクトリならindexが設定されているか、もしくはautoindexがonになっているか
            4. cgiなら実行権限を持つか
            5. bodyの大きさはclient_max_body_sizeよりも小さいか
            6. CGIを実行するわけでもないエンドポイント && POST or DELETEのときは405 <- nginx behavior
    */
   struct stat info;
    // 対象のファイル,ディレクトリの存在をチェックしつつ、infoに情報を読み込む
    if (stat(pathFromConfRoot.c_str(), &info) != 0)
        return 302;
    // 対象がディレクトリであるか確認
    if (S_ISDIR(info.st_mode))
    {
        // indexが設定されていたらそのファイルを出す。Location.cppのconfirmIndex()にてindexディレクティブが存在することは保証される
        if (location->locationSetting["index"] != "off"){
            pathFromConfRoot = location->locationSetting["index"]; //confが指すindexがあることは保証される
            // autoindexがonであってもindexが優先される。confの順序は関係なし
        }
        else if (location->locationSetting["autoindex"] == "on" && location->locationSetting["index"] == "off"){ 
            /*
                ディレクトリリストを表示するにはconfにindex off;が必要で、index off;がない時にはindex index.html;が指定される <- nginx behavior
            */
            isAutoindex = true;
            return 0;
        }
            // return sendAutoindexPage(obj->requestInfo, obj);
        // else
        //     return sendForbiddenPage(obj);//403エラー
        if (location->locationSetting["index"] == "off" && location->locationSetting["autoindex"] != "on")
            return 403;
    }
    if(isCgiRequest()){
        isCgi = true;
        if (access(pathFromConfRoot.c_str(), X_OK))
            return 403;
    }
    if (server->serverSetting.find("client_max_body_size") != server->serverSetting.end())
    {
        std::istringstream iss(server->serverSetting["client_max_body_size"]);
        unsigned long long number;
        iss >> number;
        if (getContentLength() > number)
            return 413;// return requestEntityPage(obj);
    }
    if(!isCgi && (method == "POST" || method == "DELETE"))   
        return 405;
    return 0;
}

// void Request::sendResponse(progressInfo *obj){
//     if (obj->requestInfo.getMethod() == "GET")
//     {
        // redirec ->location設定の中で最優先
        //autoindex ->sendStaticPage関数内
        // else if (isCgi(obj->requestInfo))//<- .cgi実行ファイルもMakefileで作成・削除できるようにする
        // {
        //     int pipe_c2p[2];
        //     if(pipe(pipe_c2p) < 0)
        //         throw std::runtime_error("Error: pipe() failed");
        //     obj->pipe_c2p[R] = pipe_c2p[R];
        //     obj->pipe_c2p[W] = pipe_c2p[W];
        //     obj->timerHandler = sendTimeoutPage;
        //     obj->processHandler = confirmExitStatusFromCgi;
        //     pid_t pid = fork();
        //     if(pid == 0){
        //         executeCgi(obj->requestInfo, pipe_c2p);
        //     } else if(pid > 0) {
        //         obj->childPid = pid;
        //         createNewEvent(obj->socket, EVFILT_WRITE, EV_DELETE, 0, 0, obj);
        //         createNewEvent(obj->socket, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, 10000, obj);
        //         createNewEvent(pid, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, obj);
        //     } else {
        //         obj->sendKind = FORK;
        //         sendInternalErrorPage(obj);
        //     }
        // }
        //その他の静的ファイルまたはディレクトリ
//         else
//             sendStaticPage(obj);
//     }
//     else if (obj->requestInfo.getMethod() == "POST")
//     {
//         if(obj->requestInfo.getPath() == UPLOAD)
//             postProcess(obj->requestInfo, obj);
//         else if(isCgi(obj->requestInfo))
//             postProcess(obj->requestInfo, obj);
//         else//メソッドがPOSTなのにリクエストパスがcgi以外の場合
//             sendForbiddenPage(obj);
//     }
//     else if (obj->requestInfo.getMethod() == "DELETE")
//     {
//         if (path.substr(0, 7) == UPLOAD)// リクエストパスがアップロード可能なディレクトリならファイルの削除
//             deleteProcess(obj);
//         else//メソッドがDELETEなのにリクエストパスが"/upload/"以外の場合
//             sendForbiddenPage(obj);
//     }
// }


Location* Request::getLocationSetting(){
    string location_path = selectBestMatchLocation(server->locations);
    Location *locationPtr;
    if(location_path != ""){
        locationPtr = server->locations[location_path];
    }else{
        locationPtr = new Location("");
        allocFlag = true;
        locationPtr->confirmValuesLocation();
    }
    return locationPtr;
}

string Request::selectBestMatchLocation(map<string, Location*> &locations)
{
    string bestMatch = "";
    for (map<string, Location*>::const_iterator it = locations.begin(); it != locations.end(); ++it)
    {
        if (it->second->locationSetting.find("locationPath") != it->second->locationSetting.end())
        {
            const string &locationPath = it->second->locationSetting["locationPath"];
            if (requestTarget.substr(0, locationPath.size()) == locationPath)
            {
                if (locationPath.size() > bestMatch.size())
                    bestMatch = locationPath;
            }
        }
    }
    return (bestMatch);
}


void Request::setCorrespondServer(){
    server = config->getServer(getHostName());
    if(server && server->getListenPort() != getPort())
        server = NULL;
    if(!server){
        //port一致サーバの列挙
        serversMap servers;
        config->getSamePortListenServers(getPort(), servers);
        //default_server確認
        for(serversMap::iterator it = servers.begin();it != servers.end();it++){
            if(it->second->isDefault){
                server = it->second;
                return ;
            }
        }
        //default_serverがなければindexが若いもの
        for(serversMap::iterator it = servers.begin();it != servers.end();it++){
            if(it->second->index == 0){
                server = it->second;
                return ;
            }
        }
    }
}

void Request::setCorrespondLocation(){
    location = getLocationSetting();
}

void Request::createPathFromConfRoot(){
    /*
        「configのrootディレクティブで設定されたpath」と「request lineで指定されたpath」を連結する関数。
        前提として「configのroot」の末尾には基本的に"/"が存在しないが、configでrootが"/"のみの時には空文字列になるという仕様になっている
        example: [ root & req_path ]
            [ ".." & "GET /cgi/ HTTP1.1" ]   -> "../cgi/"　(webservファイルからの相対パス)
            [ "."  &  "GET /cgi/ HTTP1.1" ]    -> "./cgi/"   (webservファイルからの相対パス)
            [ ""  &  "GET /cgi/ HTTP1.1" ]   -> "/cgi/" 
            [ "/bin" & "GET /cgi/ HTTP1.1" ] -> "/bin/cgi/" 
            [ "."  &  "GET / HTTP1.1" ] -> "./"
    */
    pathFromConfRoot = location->locationSetting["root"] + requestTarget;
    // if(pathFromConfRoot == "")
    //     pathFromConfRoot = "./";
}

string Request::getMethod(){
    return method;
}
string Request::getRequestTarget (){
    /*
        request lineに含まれるpathそのままの文字列を返す関数。
        example:
            GET /cgi/ HTTP1.1 -> "/cgi/"
            GET / HTTP1.1 -> "/"
    */
    return requestTarget ;
}
string Request::getPath (){
    /*
        「request lineに含まれるpath」と「configのroot」を連結したpathを返す関数。
    */
    return pathFromConfRoot;
}
string Request::getVersion(){
    return version;
}

VirtualServer *Request::getServer(){
    return server;
}
Location *Request::getLocation(){
    return location;
}

string Request::getHostName(){
    string directive = getField("host");
    if(directive != ""){
        strVec spDirective = split(directive, ':');
        if(directive[directive.length()-1] == '\r')
            directive = directive.substr(0, directive.length() -1);
        return spDirective.size() != 2 ? directive : spDirective[0];
    }
    return "";
}

string Request::getPort(){
    string directive = getField("host");
    if(directive != ""){
        strVec spDirective = split(directive, ':');
        if(spDirective.size() == 2){
            if(spDirective[1][spDirective[1].size()-1] == '\r'){
                return spDirective[1].substr(0, spDirective[1].size()-1);;
            }
        }else //Host = localhostの時
            return "80";
    }
    return "";
}

string Request::getSessionId(string cookieInfo){
    size_t head = cookieInfo.find("sessionid=");
    size_t tail = cookieInfo.find(";", head+1);
    head = head + strlen("sessionid=");
    string sessionId = cookieInfo.substr(head, tail - head);
    if(sessionId[sessionId.size()-1] == '\r')
        sessionId = sessionId.substr(0, sessionId.length()-1);
    return sessionId;
}

bool Request::searchSessionId(string cookieInfo){
    string sessionId = getSessionId(cookieInfo);
    ifstream ifs("request/userinfo.txt");
    if(!ifs)
        perror("UserInfo open failed");
    string line;
    while(!(ifs).eof()){
        getline(ifs, line);
        vector<string> vec = split(line, ' ');
        if(vec[0] == sessionId){
            ifs.close();
            return true;
        }
    }
    ifs.close();
    return false;
}

vector<string> Request::getUserInfo(string cookieInfo){
    string sessionId = getSessionId(cookieInfo);
    vector<string> vec;
    ifstream ifs("request/userinfo.txt");
    if(!ifs){
        perror("open userinfo.txt: ");
    }
    string line;
    while(!(ifs).eof()){
        getline(ifs, line);
        vector<string> vec = split(line, ' ');
        if(vec[0] == sessionId){
            ifs.close();
            return vec;
        }
    }
    ifs.close();
    return vec;
}

bool Request::getIsAutoindex(){
    return isAutoindex;
}
bool Request::getIsCgi(){
    return isCgi;
}
bool Request::isAllowMethod(){
    if(location->locationSetting["allow_method"] == "none")
        return true;
    std::vector<std::string>status = split(location->locationSetting["allow_method"], ' ');
    for (std::vector<std::string>::iterator it = status.begin(); it != status.end(); ++it)
    {
        if (*it == method)
            return true;
        // if (it == status.end() - 1)
        //     return false;
    }
    return false;
}

void Request::clean(){
    config = NULL;
    requestTarget = "";
    method = "";
    version = "";
    pathFromConfRoot = "";

    isCgi = false;
    isAutoindex = false;
    // unsigned long long bodySize;
    
    server = NULL;
    if(allocFlag) delete location;
    else location = NULL;
    allocFlag = false;
    createInitialStatus();
}