#include "Config.hpp"
/*========================================
        orthodox canonical form
========================================*/
serversMap Config::servers_;
std::string Config::configPath_;
Config::Config(){
}
Config::Config(std::string configPath){
    configPath_ = configPath;
    servers_ = serversMap();
}
Config::~Config(){}

std::string Config::httpDirectives_[] = {""};
std::string Config::serverDirectives_[] = {"cgi_path", "server_name", "client_max_body_size", "error_page", "listen", ""};
std::string Config::locationDirectives_[] = {"index", "root", "allow_method", "autoindex", "return", ""};

/*========================================
        public member functions
========================================*/
Config* Config::getInstance(std::string configPath){
    Config *inst;
    try{
        inst = new Config(configPath);
        readConfig(inst);
    }catch(std::bad_alloc& e){
        std::cerr<< "Error: Failed new Config() " << std::endl;
        std::exit(EXIT_FAILURE); 
    }catch(const std::runtime_error& e){
        std::cerr << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return inst;
}

void Config::readConfig(Config *inst){
    std::ifstream ifs(inst->configPath_);
    if(!ifs)
        throw std::runtime_error(inst->configPath_ + " open failed"); 
    exploreHttpBlock(&ifs);
    
}


SOCKET tcpListen(std::string port){

        struct addrinfo hints;
        struct addrinfo *result = NULL;
        SOCKET sockfd;
        
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_STREAM; // TCPソケット
        hints.ai_flags = AI_PASSIVE;

        int isError = getaddrinfo(NULL, port.c_str(), &hints, &result);//名前解決不可のため、第一引数はNULLに設定
        if(isError != 0){
            std::cerr << gai_strerror(isError) << std::endl;
            throw std::runtime_error("Error: getaddrinfo(): failed");
        }
        struct addrinfo *ai = result;
        sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if(sockfd == INVALID_SOCKET){
            freeaddrinfo(result);
            close(sockfd);
            throw std::runtime_error("Error: socket(): failed");
        }
        fcntl(sockfd, F_SETFL, O_NONBLOCK); 
        int on = 1;
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
            freeaddrinfo(result);
            close(sockfd);
            throw std::runtime_error("Error: setsockopt(): failed");
        }
        if(bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0){
            std::cerr << "bind(): " << strerror(errno) << std::endl;
            freeaddrinfo(result);
            close(sockfd);
            throw std::runtime_error("Error: bind(): failed");
        }
        freeaddrinfo(result);
        result = NULL;
        sockaddr_storage addr;
        socklen_t socklen = sizeof(addr);
        if(getsockname(sockfd, (sockaddr *)&addr, &socklen) == -1){
            close(sockfd);
            throw std::runtime_error("Error: getsockname(): failed"); 
        }
        if(listen(sockfd, 0) < 0){
            close(sockfd);
            throw std::runtime_error("Error: listen(): failed"); 
        }
        return sockfd;
}

socketSet Config::getTcpSockets(){
    socketSet set;
    for(serversMap::const_iterator it=servers_.begin();it != Config::servers_.end();it++){
        std::string tmpHostname = it->second->getServerName();
        std::string tmpPort = it->second->getListenPort();
        // const char* hostname = tmpHostname != "" ? tmpHostname.c_str() : NULL;
        // tmpHostname = tmpHostname != "" ? tmpHostname : NULL; //std::stringにNULLを代入？->segv
        // const char* port = tmpPort.c_str();
        SOCKET sockfd = tcpListen(tmpPort);
        set.insert(sockfd);
    }
    return set;
}
/*========================================
        private member functions
========================================*/
std::string searchSpecificDirective(std::ifstream copyIfs, std::string specificDirectiveName);

void Config::exploreHttpBlock(std::ifstream *ifs){
    std::string line;
    while(!(*ifs).eof()){
        std::getline(*ifs, line);
        switch(int index = getDirectiveType(line, isAppropriateDirectiveForBlock, httpDirectives_)){
            case INVALID_DIRECTIVE:
                throw std::runtime_error("Error: "+line);
                break;  
            case LOCATION_BLOCK:
                throw std::runtime_error("Error: "+line);
                break;
            case SERVER_BLOCK:
                exploreServerBlock(ifs);
            case DIRECTIVE:
                break;
            case EMPTY:
                break;
            case ERROR:
                throw std::runtime_error("Error: Invalid directive");
        }
    }
}

void Config::setServer(std::string serverName, VirtualServer *server){
    if(servers_.find(serverName) != servers_.end())
        throw std::runtime_error("server: "+serverName+" is duplicate");
    servers_[serverName] = server;
}

void Config::exploreServerBlock(std::ifstream *ifs){
    VirtualServer *server = new VirtualServer();
    std::string line;
    bool isEndBrace = false;
    while(!isEndBrace){
        if(ifs->eof())
            throw std::runtime_error("unexpect eof"); 
        std::getline(*ifs, line);
        if(isEndBraceLine(line)){
            isEndBrace = true;
            continue;
        }
        switch(int index = getDirectiveType(line, isAppropriateDirectiveForBlock, serverDirectives_)){
            case INVALID_DIRECTIVE:
                throw std::runtime_error("Error: "+line);
            case LOCATION_BLOCK:
                exploreLocationBlock(server, ifs, getDirectiveContentInLine(line, LOCATION_BLOCK));
                break;
            case SERVER_BLOCK:
                throw std::runtime_error("Error: "+line); 
            case DIRECTIVE:
                line.pop_back();
                server->setSetting(getDirectiveNameInLine(line), getDirectiveContentInLine(line, DIRECTIVE));
                break;
            case EMPTY:
                break;
            case ERROR:
                throw std::runtime_error("Error: Invalid directive");
        }
    }
    server->confirmValues();
    setServer(server->getServerName(), server);
}



void Config::exploreLocationBlock(VirtualServer *server, std::ifstream *ifs, std::string locationPath){
    Location *location = new Location(locationPath);
    std::string line;
    bool isEndBrace = false;
    while(!isEndBrace){
        if(ifs->eof())
            throw std::runtime_error("unexpect eof"); 
        std::getline(*ifs, line);
        if(isEndBraceLine(line)){
            isEndBrace = true;
            continue;
        }
        switch(int index = getDirectiveType(line, isAppropriateDirectiveForBlock, locationDirectives_)){
            case INVALID_DIRECTIVE:
                throw std::runtime_error("Error: "+line);
            case LOCATION_BLOCK:
                throw std::runtime_error("Error: "+line);
                break;
            case SERVER_BLOCK:
                throw std::runtime_error("Error: "+line); 
            case DIRECTIVE:
                location->setSetting(getDirectiveNameInLine(line), getDirectiveContentInLine(line, LOCATION_DIRECTIVE));
                break;
            case EMPTY:
                break;
            case ERROR:
                throw std::runtime_error("Error: Invalid directive");
        }
    }
    location->confirmValuesLocation();
    server->setLocation(location->getLocationPath(), location);
}



VirtualServer* Config::getServer(const std::string serverName){
    return (servers_[serverName]);
}
