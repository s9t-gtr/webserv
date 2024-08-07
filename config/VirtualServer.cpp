#include "VirtualServer.hpp"
#include <fcntl.h>
#include <unistd.h>

/*========================================
        orthodox canonical form
========================================*/
VirtualServer::VirtualServer():isDefault(false){}
VirtualServer::~VirtualServer(){}
VirtualServer::VirtualServer(const VirtualServer& other):isDefault(false){
    if(this != &other)
        *this = other;
}

VirtualServer& VirtualServer::operator=(const VirtualServer& other){
    if(this == &other)
        return *this;
    serverSetting = other.serverSetting;
    locations = other.locations;
    isDefault = other.isDefault;
    return *this;
}


/*========================================
        public member functions
========================================*/
serverMap::iterator VirtualServer::searchSetting(std::string directiveName){
    return serverSetting.find(directiveName);
}

serverMap::iterator VirtualServer::getItEnd(){
    return serverSetting.end();
}

void VirtualServer::setSetting(std::string directiveName, std::string directiveContent){
    if(serverSetting.find(directiveName) != serverSetting.end())
        throw std::runtime_error(directiveName+" is duplicate");
    serverSetting[directiveName] = directiveContent;
    if (directiveName == "client_max_body_size")
    {
        if (directiveContent.empty())
            throw std::runtime_error("Error: client_max_body_size is empty");
        std::istringstream iss(directiveContent);
        unsigned long number;
        char c;
        if (!(iss >> number) || (iss.get(c))) {
            throw std::runtime_error("Error: client_max_body_size is invalid");
    }
    }
}
std::string VirtualServer::getServerName(){
    return serverSetting["server_name"];
}
std::string VirtualServer::getListenPort(){
    if(serverSetting.find("listen") == serverSetting.end())   
        return "";
    return serverSetting["listen"];
}
void VirtualServer::setLocation(std::string locationPath, Location *location){
    if(locations.find(locationPath) != locations.end())
        throw std::runtime_error("server: "+locationPath+" is duplicate");
    locations[locationPath] = location;
}

void VirtualServer::confirmValues(){
    confirmServerName();
    confirmListenPort();
    confirmErrorPage();
    confirmCgi();
}
void VirtualServer::confirmServerName(){
    if(serverSetting.find("server_name") == serverSetting.end()){
        setSetting("server_name", "");
    }
}

void VirtualServer::confirmListenPort(){
    if(serverSetting.find("listen") == serverSetting.end()){
        setSetting("listen", "80");
        return;
    }
    size_t len = serverSetting["listen"].size();
    for(size_t i=0;i<len;i++){
        if(serverSetting["listen"][i] == '\t')
               serverSetting["listen"][i] = ' ';
    }
    for(size_t i=0;i<len;i++){
        if(!std::isdigit(serverSetting["listen"][i])){
            std::vector<std::string> vec = split(serverSetting["listen"], ' ');
            if(vec.size() == 2 && vec[1] == "default_server"){
                isDefault = true;
                serverSetting["listen"] = vec[0];
                len = vec[0].size();
            }
            else
                throw std::runtime_error("Error: listen port is invalid");
        }
    }
    int port = stoi(serverSetting["listen"]);
    if(!(0 <= port && port <= 65535))
        throw std::runtime_error("Error: listen port is out of range"); 
}

void VirtualServer::confirmErrorPage(){
    if(serverSetting.find("error_page") == serverSetting.end()){
        setSetting("error_page", "documents/404_default.html");
        return;
    }
    std::vector<std::string>status = split(serverSetting["error_page"], ' ');
    std::vector<std::string>::iterator it = status.begin();
    if(*it != "404")
        throw std::runtime_error("Error: select 404"); //error_pageの設定は404のみに対応させる
    it++;
    std::string error_page_path = *it;
    if(access(error_page_path.c_str(), F_OK | R_OK) != 0)
        throw std::runtime_error("Error: error_page does not exist or permission denied");
    serverSetting["error_page"] = error_page_path;
    it++;
    if (it != status.end())
        throw std::runtime_error("Error: invalid error_page path");
}


std::string VirtualServer::getCgiPath(){
    return serverSetting["cgi_path"];
}

void VirtualServer::confirmCgi(){
    if(serverSetting.find("cgi_path") == serverSetting.end()){
        setSetting("cgi_path", "cgi_post/upload.cgi");
    }
    if(access(getCgiPath().c_str(), F_OK | X_OK) != 0){
        throw std::runtime_error("Error: cgi_path does not exist or permission denied");
    }
}
