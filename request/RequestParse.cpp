#include "RequestParse.hpp"

RequestParse::RequestParse(std::string requestMessage){
    setMethodPathVersion(requestMessage);
    setHeadersAndBody(requestMessage);
    if (headers.find("Host") == headers.end()) {
        throw std::runtime_error("Error: Missing required 'host' header");
    }
    if (version != "HTTP/1.1")
    {
        throw std::runtime_error("Error: Invalid version");
    }
}

RequestParse::~RequestParse(){

}

RequestParse::RequestParse(const RequestParse& other){
    if(this != &other)
        *this = other;
}

RequestParse& RequestParse::operator=(const RequestParse& other){
    if(this != &other){
    method = other.method;
    path = other.path;
    version = other.version;
    headers = other.headers;
    body = other.body;

        return *this;
    }
    return *this;
}
std::string RequestParse::getRequestLine(std::string& requestMessage){
    std::string::size_type idx = requestMessage.find("\n", 0);
    if(idx == std::string::npos)
        throw std::runtime_error("Error: invalid request: non new line");
    std::string firstRow = requestMessage.substr(0, idx);
    firstRow.pop_back();
    requestMessage = requestMessage.substr(idx+1);
    return firstRow;
}

void RequestParse::setMethodPathVersion(std::string& requestMessage){
    std::string firstRow = getRequestLine(requestMessage);

    int spaceCount = 0;
    std::string::const_iterator it;
    for (it = firstRow.begin(); it != firstRow.end(); ++it) {
        if (*it == ' ') {
            spaceCount++;
        }
    }
    if (*it == ' ')
        throw std::runtime_error("Invalid request line format: Incorrect number of spaces");
    if (spaceCount != 2) {
        throw std::runtime_error("Invalid request line format: Incorrect number of spaces");
    }

    strVec splitFirstRow = split(firstRow, ' ');
    if(splitFirstRow.size() != 3)
        throw std::runtime_error("Error: invalid request: non space in first line"); 
    method = splitFirstRow[0];
    path = splitFirstRow[1];
    version = splitFirstRow[2];

}

void RequestParse::setHeadersAndBody(std::string& requestMessage){
    strVec linesVec = splitLines(requestMessage, '\n');
    strVec::iterator it;
    setHeaders(linesVec, it);
    setBody(linesVec, it);
}

strVec createHeaderAndDirective(strVec::iterator it);

void RequestParse::setHeaders(strVec linesVec, strVec::iterator& it){
    for(it=linesVec.begin();it != linesVec.end();it++){
        // std::cout << *it << std::endl;
        if(*it == "" || *it == "\r" || (*it).size() == 0)
            break;
        strVec line = createHeaderAndDirective(it);
        cutPrefixSpace(line[1]);
        headers[line[0]] = line[1];
    }
    it++;
}

void RequestParse::setBody(strVec  linesVec, strVec::iterator itFromBody) {
    if((*itFromBody).size() == 0 || itFromBody == linesVec.end()){
        body = "";
        return;
    }
    if(headers["Transfer-Encoding"] == "chunked")
        return bodyUnChunk(linesVec, itFromBody);
    body = createBodyStringFromLinesVector(linesVec, itFromBody);
}
void RequestParse::bodyUnChunk(strVec linesVec, strVec::iterator itFromBody){
    /*
        文字数が書かれた行を無視して連結するだけの処理
    */
   std::string bodyString;
   size_t mod = 0;
    for(strVec::iterator it=itFromBody;it!=linesVec.end();it++){
        if(mod % 2 == 1){
            *it = *it+'\n';
            bodyString += *it;
        }
        mod++;
    }
    body = bodyString; 
}
// POSTメソッドでリクエストボディを追加するとこの関数でエラーになってしまう
// std::string RequestParse::createBodyStringFromLinesVector(strVec linesVec, strVec::iterator itFromBody){
//     std::string bodyString;
//     for(strVec ::iterator it=itFromBody;it!=linesVec.end();it++){
//         *it = *it+'\n';
//         bodyString += *it;
//     }
//     bodyString.pop_back();
//     return bodyString;
// }

std::string RequestParse::createBodyStringFromLinesVector(strVec linesVec, strVec::iterator itFromBody){
    (void)itFromBody;//itFromBodyがエラーの原因かも？

    // linesVecの先頭からみていく。改行が二回連続したら、それ以降をリクエストボディとして格納

    std::string bodyString = "";

    bool skipMode = true; // 初めはスキップモード

    // linesVec から文字列を抽出
    for (strVec::iterator it = linesVec.begin(); it != linesVec.end(); ++it)
    {
        if (skipMode)// スキップモード中の処理
        {
            if (static_cast<int>((*it)[0]) == 13) // if (*it == "\n")ではなぜかうまくいかない
                skipMode = false; // 改行が2回連続したらスキップモードを解除
        }
        else// スキップ後の処理
            bodyString += *it; // 抽出された文字列に要素を追加
    }

    return bodyString;
}


std::string RequestParse::getMethod(){
    return method;
}
std::string RequestParse::getPath(){
    return path;
}
std::string RequestParse::getVersion(){
    return version;
}
std::string RequestParse::getHeader(std::string header){
    return headers[header];
}
std::string RequestParse::getBody(){
    return body;
}
void RequestParse::test__headers(){
    for(headersMap::iterator it=headers.begin();it!=headers.end();it++){
        std::cout << "header: " << it->first << std::endl;
        std::cout << "      directive: " << it->second << std::endl;
    }
}
strVec RequestParse::splitLines(std::string str, char sep) {
   size_t first = 0;
   std::string::size_type last = str.find_first_of(sep);
   strVec  result;
    if(last == std::string::npos){
        result.push_back(str);
        return result;
    }
   while (first < str.size()) {
     result.push_back(str.substr(first, last - first));
     first = last + 1;
     last = str.find_first_of(sep, first);
     if (last == std::string::npos) last = str.size();
   }
   return result;
 }

std::string createDirectiveFromSplitVec(strVec line){
    size_t idx = 0;
    std::string directive;
    for(strVec::iterator it=line.begin();it!=line.end();it++){
        if(idx > 0){
            *it+=':';
            directive += *it;
        }
        idx++;
    }
    directive.pop_back();
    return directive;
}

strVec createHeaderAndDirective(strVec::iterator it){
    strVec line = split(*it, ':');
    if(line.size() < 2){
        std::cerr << *it << std::endl;
        throw std::runtime_error("Unexpected Error: Encountered header not delimited by ':'");//Notice: このケースがあるならParseの方法を変更しなければならない
    }
    if(line.size() > 2)
        line[1] = createDirectiveFromSplitVec(line);
    return line;
}

std::string RequestParse::getHostName(){
    std::string directive = getHeader("Host");
    if(directive != ""){
        strVec spDirective = split(directive, ':');

        return spDirective.size() != 2 ? "localhost" : spDirective[0];
    }
    return "";
}