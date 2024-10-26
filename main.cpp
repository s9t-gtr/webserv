#include "config/Config.hpp"
#include "HttpConnection/HttpConnection.hpp"
#include <sys/wait.h>



#define W 1
#define R 0
int main(int argc, char **argv){
    if(argc != 2){
        std::cout << "Error: invalid execute" << std::endl;
        return 1;
    }
    try{
        Config *conf = Config::getInstance(argv[1]);
        std::set<int> tcpSocketSet = conf->getTcpSockets();
        // HttpConnection* connection = HttpConnection::getInstance(tcpSocketSet);
        HttpConnection httpConnection(tcpSocketSet);
        httpConnection.startEventLoop(conf);
    }catch(std::runtime_error& e){
        std::cerr << e.what() << std::endl;
        exit(1);
    }catch(std::bad_alloc& e){
        std::cerr << e.what() << std::endl;
        exit(1);
    }
}

