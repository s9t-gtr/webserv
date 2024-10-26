#include "HttpMessageParser.hpp"

HttpMessageParser::HttpMessageParser(DetailStatus::StartLineReadStatus startLineReadStatus){
    readingStatus = StartLine;
    detailStatus.startLineStatus = startLineReadStatus;
    isWatingLF = false;
    isAfterFieldLineCRLF = false;
    isSyntaxErrorDetected = false;

    contentLength = 0;
    responseStatusCode = 0;
}

HttpMessageParser::~HttpMessageParser(){}

void HttpMessageParser::readBufferAndParse(char *buffer, size_t bytesReceived){
    std::string readingBuffer = std::string(buffer, buffer + bytesReceived);
    // convertCarriageReturnToSpace(readingBuffer); //RFC 9112 (2.2)
    if(!isOnlyAsciiCharacter(readingBuffer)) //RFC 9112 (2.2)
        throw std::runtime_error("Error: invalid request: Contains non-ascii characters");
    for(std::string::size_type i = 0; i < readingBuffer.size(); i++){
        ReadingStatus status = getReadingStatus();
        if(status == StartLine){
            parseStartLine(readingBuffer[i]);
        } else if(status == Headers){
            parseHeaders(readingBuffer[i]);
        } else if(status == Body){
            parseBody(readingBuffer[i]);
        }
    }
}




std::string cutSpaces(std::string str, bool isTailToHead){
    std::string::size_type length = str.size();
    if(isTailToHead){
        for(std::string::size_type i = length - 1; i > 0; i--){
            if(std::isspace(str[i])){
                length--;
            }
        }
        return str.substr(0, length);
    }
    return str;
}

std::string cutZeroFromHead(std::string str){
    std::string::size_type length = str.size();
    std::string::size_type i;
    for(i = 0; i < length; i++){
        if(str[i] == '0')
            i++;
        else
            break;
    }
    return str.substr(i, length);
}

bool isOnlyDigitString(std::string str){
    std::string::size_type length = str.size();
    for(std::string::size_type i = 0; i < length; i++){
        if(!std::isdigit(str[i]))
            return false;
    }
    return true;
}

void HttpMessageParser::parseHeaderContentLength(){
    if(headers.find("content-length") != headers.end()){
        if(!isOnlyDigitString(headers["content-length"]))
            syntaxErrorDetected();
        if(headers["content-length"][0] == '0')
            headers["content-length"] = cutZeroFromHead(headers["content-length"]);
        std::stringstream ss(headers["content-length"]);
        ss >> contentLength;
    }
}

void HttpMessageParser::parseHeaders(char c){
    /*
        field nameは常に小文字に変換して保持する
    */
    static std::string fieldName;
    static std::string fieldValue;
    switch(getDetailStatus().headerStatus){
        case DetailStatus::FirstLineForbiddenSpace:
            if(std::isspace(c))
                syntaxErrorDetected();
            toTheNextStatus(); // to FieldName
            fieldName += std::tolower(c);
            break;
        case DetailStatus::SpaceBeforeFieldName:
            if(std::isspace(c)){
                if(c == CHAR_CR){
                    setIsWatingLF(true);
                    setDetailStatus(DetailStatus::Crlf);
                    return ;
                }
                if(c == CHAR_LF){
                    setDetailStatus(DetailStatus::Crlf);
                    return ;
                }
                setIsAfterFieldLineCRLF(false);
                return ;
            }
            setIsAfterFieldLineCRLF(false);
            toTheNextStatus();
            fieldName += std::tolower(c);
            break;
        case DetailStatus::FieldName:
            if(c == ':'){
                toTheNextStatus();
                return ;
            }
            if(std::isspace(c)){
                if(c == CHAR_CR){
                    setIsWatingLF(true);
                    setDetailStatus(DetailStatus::Crlf);
                    return ;
                }
                if(c == CHAR_LF){
                    setDetailStatus(DetailStatus::Crlf);
                    return ;
                }
                syntaxErrorDetected();
            }
            fieldName += std::tolower(c);
            break;
        case DetailStatus::SpaceBeforeFieldValue:
            if(std::isspace(c)){
                if(c == CHAR_CR){
                    setIsWatingLF(true);
                    setDetailStatus(DetailStatus::Crlf);
                }
                if(c == CHAR_LF)
                    setDetailStatus(DetailStatus::Crlf);
                return ;
            }
            toTheNextStatus();
            fieldValue += c;
            break;
        case DetailStatus::FieldValue:
            if(c == CHAR_CR){
                setIsWatingLF(true);
                toTheNextStatus();
                return ;
            }
            if(c == CHAR_LF){
                toTheNextStatus();
                return ;
            }
            fieldValue += c;
            break;
        case DetailStatus::Crlf:
            if(getIsWatingLF()){
                setIsWatingLF(false);
                if(c != CHAR_LF)
                    syntaxErrorDetected();
            }
            if(getIsAfterFieldLineCRLF()){ // headers終了判定
                parseHeaderContentLength();
                // transfer-encoding 考慮処理

                if(contentLength > 0)
                    setReadingStatus(Body);
                else
                    setReadingStatus(Complete);
                return ;
            }
            setIsAfterFieldLineCRLF(true);
            if(fieldName != ""){
                headers[fieldName] = cutSpaces(fieldValue, true);
            }
            fieldName = "";
            fieldValue = "";
            toTheNextStatus();
            break;
        default:
            break;

    }
    
}



void HttpMessageParser::parseBody(char c) {
    // if(headers["transfer-encoding"] == "chunked"){
    //     bodyUnChunk(linesVec, itFromBody);
    // }else{
    //     body += buffer;
    // }
    // if(buffer.size() >= getField["content-length"]){
    //     body = body.substr(0, getField["content-length"]);
    //     readingStatus = Complete;
    // }
    if(contentLength-- > 0)
        body += c;
    else
        setReadingStatus(Complete);
}

// void HttpMessageParser::bodyUnChunk(strVec linesVec, strVec::iterator itFromBody){
//     /*
//         文字数が書かれた行を無視して連結するだけの処理
//     */
//    std::string bodyString;
//    size_t mod = 0;
//     for(strVec::iterator it=itFromBody;it!=linesVec.end();it++){
//         if(mod % 2 == 1){
//             *it = *it+'\n';
//             bodyString += *it;
//         }
//         mod++;
//     }
//     body = bodyString; 
// }

void HttpMessageParser::toTheNextStatus(){
    switch(readingStatus){
        case StartLine:
            switch (detailStatus.startLineStatus.requestLineStatus){
                case DetailStatus::StartLineReadStatus::SpaceBeforeMethod:
                    detailStatus.startLineStatus.requestLineStatus = DetailStatus::StartLineReadStatus::Method;
                    break;
                case DetailStatus::StartLineReadStatus::Method:
                    detailStatus.startLineStatus.requestLineStatus = DetailStatus::StartLineReadStatus::SpaceBeforeRequestTarget;
                    break;
                case DetailStatus::StartLineReadStatus::SpaceBeforeRequestTarget:
                    detailStatus.startLineStatus.requestLineStatus = DetailStatus::StartLineReadStatus::RequestTarget;
                    break;
                case DetailStatus::StartLineReadStatus::RequestTarget:
                    detailStatus.startLineStatus.requestLineStatus = DetailStatus::StartLineReadStatus::SpaceBeforeVersion;
                    break;
                case DetailStatus::StartLineReadStatus::SpaceBeforeVersion:
                    detailStatus.startLineStatus.requestLineStatus = DetailStatus::StartLineReadStatus::Version;
                    break;
                case DetailStatus::StartLineReadStatus::Version:
                    detailStatus.startLineStatus.requestLineStatus = DetailStatus::StartLineReadStatus::TrailingSpaces;
                    break;
                case DetailStatus::StartLineReadStatus::TrailingSpaces:
                    readingStatus = Headers;
                    detailStatus.headerStatus = DetailStatus::FirstLineForbiddenSpace; 
                    break;
                default:
                    break;
            }
            break;
        case Headers:
            switch (detailStatus.headerStatus){
                case DetailStatus::FirstLineForbiddenSpace: // 
                    detailStatus.headerStatus = DetailStatus::FieldName;
                    break;
                case DetailStatus::SpaceBeforeFieldName:
                    detailStatus.headerStatus = DetailStatus::FieldName;
                    break;
                case DetailStatus::FieldName:
                    detailStatus.headerStatus = DetailStatus::SpaceBeforeFieldValue;
                    break;  
                case DetailStatus::SpaceBeforeFieldValue:
                    detailStatus.headerStatus = DetailStatus::FieldValue;
                    break;
                case DetailStatus::FieldValue:
                    detailStatus.headerStatus = DetailStatus::Crlf;
                    break;
                // case DetailStatus::SpaceBeforeCrlf:
                //     detailStatus.headerStatus = DetailStatus::Crlf;
                //     break;
                case DetailStatus::Crlf:
                    detailStatus.headerStatus = DetailStatus::SpaceBeforeFieldName;
                    break;
                default:
                    break;
            }
            break;
        case Body:
            readingStatus = Complete;
            // switch (detailStatus.bodyStatus){
            //     // case Content:
            //         break;
            // }
            break;
        case Complete:
            break;
        default:
            break;

    }
}

HttpMessageParser::ReadingStatus HttpMessageParser::getReadingStatus(){
    return readingStatus;
}
void HttpMessageParser::setReadingStatus(ReadingStatus newReadingStatus){
    readingStatus = newReadingStatus;
}

HttpMessageParser::DetailStatus HttpMessageParser::getDetailStatus(){
    return detailStatus;
}
void HttpMessageParser::setDetailStatus(DetailStatus::StartLineReadStatus::RequestLineReadStatus newDetailStatus){
    detailStatus.startLineStatus.requestLineStatus = newDetailStatus;
}
void HttpMessageParser::setDetailStatus(DetailStatus::StartLineReadStatus::StatusLineReadStatus newDetailStatus){
    detailStatus.startLineStatus.statusLineStatus = newDetailStatus;
}
void HttpMessageParser::setDetailStatus(DetailStatus::HeaderReadStatus newDetailStatus){
    detailStatus.headerStatus = newDetailStatus;
}
void HttpMessageParser::setDetailStatus(DetailStatus::BodyReadStatus newDetailStatus){
    detailStatus.bodyStatus = newDetailStatus;
}

bool HttpMessageParser::getIsWatingLF(){
    return isWatingLF;
}
void HttpMessageParser::setIsWatingLF(bool value){
    isWatingLF = value;
}
bool HttpMessageParser::getIsAfterFieldLineCRLF(){
    return isAfterFieldLineCRLF;
}
void HttpMessageParser::setIsAfterFieldLineCRLF(bool value){
    isAfterFieldLineCRLF = value;
}

void HttpMessageParser::syntaxErrorDetected(StatusCode_t statusCode){
    responseStatusCode = statusCode;
    isSyntaxErrorDetected = true;
}
bool HttpMessageParser::getSyntaxErrorDetected(){
    return isSyntaxErrorDetected;
}
unsigned int HttpMessageParser::getResponseStatusCode(){
    return responseStatusCode;
}

std::string::size_type HttpMessageParser::getContentLength(){
    return contentLength;
}


BadRequest_400::BadRequest_400(const char* error_message): std::invalid_argument(error_message){}
const char* BadRequest_400::what() const throw() {
    return std::invalid_argument::what();
}

Found_302::Found_302(const char* error_message): std::invalid_argument(error_message){}
const char* Found_302::what() const throw() {
    return std::invalid_argument::what();
}

NotAllowed_405::NotAllowed_405(const char* error_message): std::invalid_argument(error_message){}
const char* NotAllowed_405::what() const throw() {
    return std::invalid_argument::what();
}



bool HttpMessageParser::isOnlyAsciiCharacter(std::string line){
    for(std::string::size_type i = 0; i < line.size(); i++){
        if(!isascii(line[i]))
            return false;
    }
    return true;
}   

std::string HttpMessageParser::getField(std::string fieldName){
    if(headers.find(fieldName) == headers.end())
        return "";
    return headers[fieldName];
}

std::string HttpMessageParser::getBody(){
    return body;
}