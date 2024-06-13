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
