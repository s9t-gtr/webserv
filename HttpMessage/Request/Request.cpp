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
    isQueryString = false;
	startLineLength = 0;
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
   if(!isascii(c)){
        syntaxErrorDetected();
    }
	startLineLength += 1;
	if (startLineLength > 8192) {
		std::cout << "startLineLength: " << startLineLength << std::endl;
		syntaxErrorDetected(414);
		return;
	}
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
				if(requestTarget.size() > 0 && requestTarget[0] != '/'){
					requestTarget = "";
					syntaxErrorDetected();
                    setReadingStatus(Complete);
				}
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
            if (c == '?') { 
                isQueryString = true; 
                return ;
            }
            if (isQueryString) {
                queryString += c;
                return ;
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
			//if(c == 'H'){
            toTheNextStatus();
            version += c;
            return ;
           // }
           // if(c == '%')
           //     syntaxErrorDetected();
           // syntaxErrorDetected(302);
           // break;
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
	if(getResponseStatusCode() == 414) return 414;
	if(getField("host") == "") return 400;
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
	//requestTargetがどんなパターンの不正なリクエストでも、HttpVersionの先頭が'H'かつ不正な場合はそれに応じたStatusCodeのレスポンスを返す
	//HttpVersionの先頭が'H'でない場合、その文字列をrequestTargetの一部として扱うため、validateRequestTarget()の結果に従う
	StatusCode_t validateHttpVersionResult = validateHttpVersion();
	if (validateHttpVersionResult != 0) { 
		return validateHttpVersionResult; 
	}
	StatusCode_t validateRequestTargetResult = validateRequestTarget();
    return validateRequestTargetResult;
}

bool Request::isCgiRequest()
{
    std::string::size_type pathLength = pathFromConfRoot.size();
    const std::string pythonExtension = ".py";
    const std::string cgiExtension = ".cgi";
    if(pathFromConfRoot.find(".") == std::string::npos)
        return false;
    if(pathFromConfRoot.substr(pathLength > cgiExtension.size() ? pathLength - cgiExtension.size() : 0) == cgiExtension)
        return true;
    if(pathFromConfRoot.substr(pathLength > pythonExtension.size() ? pathLength - pythonExtension.size() : 0) == pythonExtension)
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
	
   	//parse時requestTarget読み取り時点で不正が発覚した場合はversionの有無,正誤に関わらず400を返す.
   	if (requestTarget == "") { return 400; }
    if (stat(pathFromConfRoot.c_str(), &info) != 0) {
		//設定されたerror_pageが存在しない場合は302
        if (stat(getServer()->serverSetting["error_page"].c_str(), &info) != 0){
            return 302;
        }
        return 404;
    }
	
       
	
    // 対象がディレクトリであるか確認
    if (S_ISDIR(info.st_mode))
    {
		if (method == "POST")
			return 403;
		//Directoryなのに末尾に/がない場合は404, 404がない時は301
		if (pathFromConfRoot[pathFromConfRoot.size() - 1] != '/'){
			return 301;
		}
        // indexが設定されていたらそのファイルを出す。Location.cppのconfirmIndex()にてindexディレクティブが存在することは保証される
        if (location->locationSetting["index"] != "off"){
            pathFromConfRoot = location->locationSetting["index"]; //confが指すindexがあることは保証されている
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
	if (server->serverSetting.find("client_max_body_size") != server->serverSetting.end())
    {
        std::istringstream iss(server->serverSetting["client_max_body_size"]);
        unsigned long long number;
        iss >> number;
		std::cout << "number: " << number << std::endl;
        if (getContentLength() > number)
            return 413;// return requestEntityPage(obj);
    }
    if(isCgiRequest() || method == "POST"){
        isCgi = true;
        if (access(pathFromConfRoot.c_str(), X_OK) != 0)
            return 403;
    }
    
    if(!isCgi && (method == "POST" || method == "DELETE"))   
        return 405;
	if(access(pathFromConfRoot.c_str(), R_OK) != 0) {
		return 403;
	}
    return 0;
}

std::string removeLeadingZeros(const std::string& str);
StatusCode_t getUntilNotDigitStatus(std::string major) ;
StatusCode_t Request::validateHttpVersion(){
	if(version == "") { return 0; }
	if(version.size() > 0 && version[0] != 'H') { 
		pathFromConfRoot += version;
		return 0; 
	}
	if(version.size() < 5) { return 400; }
    if(version.substr(0, 5) != "HTTP/") { return 400; }
    if(version.size() < 7) { return 400; }
    std::string digits = version.substr(5);
	if(digits[digits.size() - 1] == '.') { return 400; }
    std::string::size_type commaIndex = digits.find(".");
    if( commaIndex == std::string::npos ) { return 400; }
    //NOTICE: .の存在は確認したが数値チェック省略
	
    std::vector<std::string> splitByComma = split(digits, '.');
    if (splitByComma.size() != 2) { return 400; }

    for(size_t i = 0; i < splitByComma.size(); i++){
		//istringstreamは先頭の符号を読み取ってしまうので事前に弾く
		if( splitByComma[i].size() > 0 && (splitByComma[i][0] == '+' || splitByComma[i][0] == '-')) {
			return 400; 
		}
		//major
		if (i == 0) {
			/*
			 	majorはprefixの0を許容しない。
			 */
			if( splitByComma[i].find_first_not_of('0') != 0) { return 400; }
			//999aのとき505を返したい
			StatusCode_t untilNotDigitStatus = getUntilNotDigitStatus(splitByComma[i]);
			if (untilNotDigitStatus != 0) { return untilNotDigitStatus; }
			std::istringstream iss(splitByComma[i]);
    	    int number;
			if (!(iss >> number) || !iss.eof()) {
				//int型でoverflowする値や文字が入っている場合
				return 400;
			}
			if (number >= 2) { 
				return 505; 
			}
		}
		//minor
		if (i == 1) {
			/*
				//minorはprefixの0を許容する。それらを省略して3桁以内の数値ならVersionとして成立する
			 */
			std::istringstream iss(splitByComma[i]);
    	    int number;
			if (!(iss >> number) || !iss.eof()) {
				//int型でoverflowする値や文字が入っている場合
				return 400;
			}
			if (number > 999) { return 400; }
		}
    }
	return 0;
}

std::string removeLeadingZeros(const std::string& str) {
    size_t pos = str.find_first_not_of('0'); // '0' 以外の最初の位置を探す
    if (pos == std::string::npos) {
        return "0"; // すべて '0' なら "0" を返す
    }
    return str.substr(pos);
}

StatusCode_t getUntilNotDigitStatus(std::string major) {
	std::string untilNotDigit = "";
	for(size_t i = 0; i < major.size(); i++){
		if(!isdigit(major[i])) { break; }
		untilNotDigit += major[i];
	}
	std::istringstream issUntilNotDigit(untilNotDigit);
    int numberUntilNotDigit;
	if (!(issUntilNotDigit >> numberUntilNotDigit) || !issUntilNotDigit.eof()) {
		return 400;
	}
	if (numberUntilNotDigit >= 2) { return 505; }
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
        // if (it->second->path != "")
        // {
            const string &locationPath = it->second->path;
            if (requestTarget.substr(0, locationPath.size()) == locationPath)
            {
                if (locationPath.size() > bestMatch.size())
                    bestMatch = locationPath;
            }
        // }
    }
    return (bestMatch);
}


void Request::setCorrespondServer(){
    serversMap listenPortServers = config->getServerByPort(getPort());
	for(serversMap::iterator it = listenPortServers.begin();it != listenPortServers.end();it++){
		if(it->second->getServerName() == getHostName()){
			server = it->second;
			return ;
		}
	}
	if (listenPortServers.size() >= 1) {
		server = listenPortServers.begin()->second;
		return;
	}
	server = config->getDefaultServer();
	return ;
   // if(server && server->getListenPort() != getPort())
   //     server = config->getServer("");
   // if(!server){
   //     //port一致サーバの列挙
   //     serversMap servers;
   //     config->getSamePortListenServers(getPort(), servers);
   //     //default_server確認
   //     for(serversMap::iterator it = servers.begin();it != servers.end();it++){
   //         if(it->second->isDefault){
   //             server = it->second;
   //             return ;
   //         }
   //     }
   //     //default_serverがなければindexが若いもの
   //     for(serversMap::iterator it = servers.begin();it != servers.end();it++){
   //         if(it->second->index == 0){
   //             server = it->second;
   //             return ;
   //         }
   //     }
   // }
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
string Request::getQueryString() {
    return queryString;
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
    string hostField = getField("host"); //事前に""になるケースは弾いている
	//前処理
    if(hostField[hostField.length()-1] == '\r')
        hostField = hostField.substr(0, hostField.length() -1);
	//抽出
    strVec spHostField = split(hostField, ':');
	if (spHostField.size() == 1 || spHostField.size() == 2){
		return spHostField[0];
	}
	return "";
}

string Request::getPort(){
    string hostField = getField("host"); //事前に""になるケースは弾いている
	//前処理
    if(hostField[hostField.length()-1] == '\r')
        hostField = hostField.substr(0, hostField.length() -1);
	//抽出
    strVec spHostField = split(hostField, ':');
    if(spHostField.size() == 2){
		return spHostField[1]; //NOTICE: 数値チェック必要? このままだと文字が入りうる
    }else //portが指定されていない場合はhttpなので80
        return "80";
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
    if(location->locationSetting["allow_method"] == "none") {
		// 設定がないならGET, POST, DELETEのみ許可
		if (method == "GET" || method == "POST" || method == "DELETE")
			return true;
        return false;
	}
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
