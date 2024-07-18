#ifndef __VIRTUALSERVER_HPP_
# define __VIRTUALSERVER_HPP_

#include <iostream>
#include <map>
#include <sstream>
#include "Location.hpp"

typedef std::map<std::string, std::string> serverMap;
typedef std::map<std::string, Location*> locationsMap;

#define ERROR_PAGE_404 "documents/404.html"
class Location;
class VirtualServer{
    public:
        serverMap serverSetting;
        locationsMap locations;
        bool isDefault ;
        size_t index; //confの中で上から何番目
    public:
        VirtualServer();
        ~VirtualServer();
        VirtualServer(const VirtualServer& other);
        VirtualServer& operator=(const VirtualServer& other);
    public:
        serverMap::iterator searchSetting(std::string directiveName);
        void setSetting(std::string directiveName, std::string directiveContent);
        std::string getServerName();
        std::string getListenPort();
        serverMap::iterator getItEnd();
        void setLocation(std::string locationPath, Location *location);
        std::string getCgiPath();
    public:
        void confirmValues();
    private:
        void confirmServerName();
        void confirmListenPort();
        void confirmErrorPage();
        void confirmCgi();
};

#endif