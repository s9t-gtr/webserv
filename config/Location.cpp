#include "Location.hpp"

/*========================================
        orthodox canonical form
========================================*/
Location::Location(std::string locationPath): path(locationPath){}
Location::~Location(){}
Location::Location(const Location& other){
    if(this != &other)
        *this = other;
}
Location& Location::operator=(const Location& other){
    if(this == &other)
        return *this;
    locationSetting = other.locationSetting;
    path = other.path;
    return *this;
}


/*========================================
        public member functions
========================================*/
locationMap::iterator Location::searchSetting(std::string directiveName){
    return locationSetting.find(directiveName);
}

locationMap::iterator Location::getItEnd(){
    return locationSetting.end();
}

void Location::setSetting(std::string directiveName, std::string directiveContent){
    if(locationSetting.find(directiveName) != locationSetting.end())
        throw std::runtime_error(directiveName+" is duplicate");
    locationSetting[directiveName] = directiveContent;
}
std::string Location::getLocationPath(){
    locationSetting["locationPath"] = path;//この行追加で複数location格納成功
    return locationSetting["locationPath"];
}

void Location::confirmValuesLocation(){
    confirmIndex();
    confirmRoot();
    confirmAllowMethod();
    confirmAutoindex();
    confirmReturn();
}

void Location::confirmIndex()
{
    if(locationSetting.find("index") == locationSetting.end())
        setSetting("index", "none");
}

void Location::confirmRoot()
{
    if(locationSetting.find("root") == locationSetting.end())
        setSetting("root", "");
    size_t len = locationSetting["root"].length();
    if(0 < len && locationSetting["root"][len-1] != '/'){
        /*
            "document"や".."を指定された時に末尾に/をつける。
            - request path.substr(1)と繋げる際にwebservパスからの相対パスを表現、rootに絶対パスを指定されても同じ連結方法でアクセスできる
                root: "..", request path: "/cgi/"の時 ../ + cgi/
                root: "/", request path: "/cgi/"の時  / + cgi/
            - best match Locationを見つける時にはrequest path のgetRawPath()を使用する
        */
        locationSetting["root"] += "/";
    }
}

void Location::confirmAllowMethod()
{
    if(locationSetting.find("allow_method") == locationSetting.end())
        setSetting("allow_method", "none");
}

void Location::confirmAutoindex()
{
    if(locationSetting.find("autoindex") == locationSetting.end())
        setSetting("autoindex", "off");
    if (locationSetting["autoindex"] != "on" && locationSetting["autoindex"] != "off")
        throw std::runtime_error("error: autoindex should be on or off");
}

void Location::confirmReturn()
{
    if(locationSetting.find("return") == locationSetting.end())
        setSetting("return", "none");
}
