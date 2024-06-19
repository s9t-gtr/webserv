#ifndef __Location_HPP_
# define __Location_HPP_

#include <iostream>
#include <map>
#include "utils.hpp"

typedef std::map<std::string, std::string> locationMap;

class Location{
    public:
        std::string path;
        locationMap locationSetting;
        
    public:
        Location(std::string path);
        ~Location();
        Location(const Location& other);
        Location& operator=(const Location& other);
    public:
        locationMap::iterator searchSetting(std::string directiveName);
        void setSetting(std::string directiveName, std::string directiveContent);
        std::string getLocationPath();
        locationMap::iterator getItEnd();
        void confirmValuesLocation();
        void confirmIndex();
        void confirmRoot();
        void confirmAllowMethod();
        void confirmAutoindex();
        void confirmReturn();
};

#endif