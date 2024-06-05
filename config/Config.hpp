#ifndef __CONFIG_HPP_
# define __CONFIG_HPP_

#include <fcntl.h>
#include <fstream>
#include <map>
#include <set>
#include <sys/event.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include "VirtualServer.hpp"
#include "Location.hpp"
#include "utils.hpp"

#define WARNING "WARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNING"


#define SOCKET int
#define INVALID_SOCKET -1 
#define STORE_OK true;
#define DUP false;

typedef std::map<std::string, VirtualServer*> serversMap;
typedef std::map<std::string, std::string> strMap;
typedef std::vector<std::string> strVec;
typedef std::set<int> socketSet;

class Config{
    public:
        static serversMap servers_;
        static std::string configPath_;
        static std::string httpDirectives_[]; 
        static std::string serverDirectives_[]; 
        static std::string locationDirectives_[]; 
        Config();
    
    private:
        Config(std::string configPath);
        Config(const Config& other);
        Config& operator=(const Config& other);
        ~Config();
    public:
        static Config* getInstance(std::string configPath);
        static void readConfig(Config *inst);
        socketSet getTcpSockets();
        VirtualServer getServer(const std::string serverName);
    private:
        static void exploreHttpBlock(std::ifstream *ifs);
        static void exploreServerBlock(std::ifstream *ifs);
        static void exploreLocationBlock(VirtualServer *server, std::ifstream *ifs, std::string location);
        static void setServer(std::string serverName, VirtualServer *server);
};

#endif