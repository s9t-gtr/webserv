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

#define SOCKET int
#define INVALID_SOCKET -1 
#define STORE_OK true;
#define DUP false;
#define DEBUG ""

//colors
#define LIGHT_GREEN "\033[1;32m"
#define BRIGHT_GREEN "\033[92m"
#define MAGENTA "\033[0;35m"
#define BRIGHT_MAGENTA "\033[95m"
#define RED "\033[0;31m"
#define ORANGE "\033[38;5;208m"  // 256色モードでのオレンジ
#define PINK "\033[38;5;205m" 
#define LIGHT_BLUE "\033[1;34m"
#define BLUE "\033[0;34m"
#define CYAN "\033[36m"
//format
#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"
#define RESET "\033[0m"

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
        VirtualServer* getServer(const std::string serverName);
        serversMap getSamePortListenServers(std::string port, serversMap &map);
    private:
        static void CheckConfigStatus(std::string configPath);
        static void exploreHttpBlock(std::ifstream *ifs);
        static void exploreServerBlock(std::ifstream *ifs);
        static void exploreLocationBlock(VirtualServer *server, std::ifstream *ifs, std::string location);
        static void setServer(std::string serverName, VirtualServer *server);
};

#endif