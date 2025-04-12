#include "utils.hpp"

bool isEndBraceLine(std::string line){ 
    /*
    そのlineに"}"が一つだけならそれは何かのendBraceであるという保証
    */
    int endBrace = 0;
    for(size_t i=0;i<line.size();i++){
        if(line[i] == '}')
            endBrace++;
        else if(line[i] != ' ' && line[i] != '\t')
            return false;
    }
    if(endBrace == 1)
        return true;
    return false;
}
size_t prefixSpaceCount(std::string line){
    size_t i = 0;
    while(line[i] == ' ' || line[i] == '\t'){
        i++;
    }
    return i;
}
void cutPrefixSpace(std::string& line){
    size_t i = 0;
    while(line[i] == ' ' || line[i] == '\t'){
        i++;
    }
    line = line.substr(i);
}

std::string getDirectiveNameInLine(std::string line){
    size_t i = 0;

    cutPrefixSpace(line);
    while(line[i] != ' ' && line[i] != '\n' && line[i] != '\t'){
        i++;
        if(line[i] == '{')
            break;
    }
    return line.substr(0, i);
}

std::string getDirectiveContentInLine(std::string line, int type){
   //この関数を使う時にはcontentを持つdirectiveであることが保証されている必要がある 
    size_t len = line.size();
    if(type == LOCATION_BLOCK){
       if(line.substr(0, 10) == "\tlocation "){//行の先頭のタブを含めるように修正
            std::string path = line.substr(10, len - (10 + 1));//substr 関数の第二引数には取得したい文字列の長さを指定する
            return path;
       }
    }
    if(type == LOCATION_DIRECTIVE){
        size_t nameLen = getDirectiveNameInLine(line).size() + prefixSpaceCount(line);
            return line.substr(nameLen+1, len - (nameLen+1 + 1));
    }
    size_t nameLen = getDirectiveNameInLine(line).size() + prefixSpaceCount(line);
    return line.substr(nameLen+1, len - 2);
}

bool isLocationDirective(std::string line){
    size_t len = line.size();
    size_t i = 0;
    if(line[len-1] != '{')
        return false;
    cutPrefixSpace(line);
    if(line.substr(0, 9) == "location "){
        std::string path = line.substr(10, len-1-10); 
        while(path[i]){ //「正常なpathである＝pathがあるべき場所に１つの文字列が入っていること」とする。
            if(path[i] == ' ' || path[i] == '\t')
                return false;
            i++;
        }
    }
    return true;
}

#define ALL_SPACE_STRING true;
#define STRING false;
static bool isAllSpace(std::string line){
    size_t len = line.size();
    for(size_t i=0;i<len;i++){
        if(line[i] != ' ' && line[i] != '\t')
            return STRING;
    }
    return ALL_SPACE_STRING;
}

int getDirectiveType(std::string line, bool(*isAppropriateDirectiveForBlock)(std::string, std::string[]), std::string AppropriateDirectices[]){
    if(line.empty() || isAllSpace(line)){
        return EMPTY;
    }else if(line == "server{"){
        return SERVER_BLOCK;
    }else if(isLocationDirective(line)){
        return LOCATION_BLOCK;
    }else if(isAppropriateDirectiveForBlock(line, AppropriateDirectices)){
        return DIRECTIVE;
    }
    return ERROR;
}



bool isAppropriateDirectiveForBlock(std::string line, std::string usableDirectives[]){
    size_t i = 0;
    size_t len = line.size();
    if(line[len-1] != ';')
        throw std::runtime_error("Error: "+line + ": is Invalid");
    std::string DirectiveName = getDirectiveNameInLine(line);
    while(usableDirectives[i] != ""){
        if(usableDirectives[i] == DirectiveName)
            return true;
        i++;
    }
    return false;
}

std::vector<std::string> split(std::string str, char sep) {
   size_t first = 0;
   std::string::size_type last = str.find_first_of(sep);
   std::vector<std::string> result;
    if(last == std::string::npos){
        result.push_back(str);
        return result;
    }
   while (first < str.size()) {
     if(str.substr(first, last - first) != "")
        result.push_back(str.substr(first, last - first));
     first = last + 1;
     last = str.find_first_of(sep, first);
     if (last == std::string::npos) last = str.size();
   }
   return result;
 }
