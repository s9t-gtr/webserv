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



    // std::cout << "----------------------------" << std::endl;
    // std::cout << directiveContent << std::endl;
    // std::cout << "----------------------------" << std::endl;




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
        setSetting("root", "../documents/");
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
