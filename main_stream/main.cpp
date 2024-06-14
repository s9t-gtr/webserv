#include "../config/Config.hpp"
#include "../request/RequestParse.hpp"
#include "../HttpConnection/HttpConnection.hpp"
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
        // std::set<int> tcpSocketSet = conf->getTcpSockets();
        // HttpConnection* connection = HttpConnection::getInstance(tcpSocketSet);
        // connection->startEventLoop(conf, tcpSocketSet);



        (void)conf;
        //configureからパースして格納したバーチャルサーバーの情報を出力するテスト
        for (serversMap::const_iterator it = Config::servers_.begin(); it != Config::servers_.end(); ++it)
        {
            //configクラスのstd::map<std::string, VirtualServer*> serversMap 型変数 servers_ のfirstの中身を出力
            std::cout << "以下Config::servers_のfirst(サーバー名)" << std::endl << it->first << std::endl;

            std::cout << std::endl;
            std::cout << "以下Config::servers_のsecond(VirtualServer*)のserverMap serverSettingの中身" << std::endl;

            //configクラスのservers_のsecond, VirtualServer*の中のserverSettingを出力
            serverMap serverSetting = it->second->serverSetting;
            for (serverMap::const_iterator it2 = serverSetting.begin(); it2 != serverSetting.end(); ++it2)
            {
                std::cout << it2->first << "->" << it2->second << std::endl;
            }

            std::cout << std::endl;

            //configクラスのservers_のsecond, VirtualServer*の中のlocationsMap locations;を深堀りしていく
            locationsMap locations = it->second->locations;
            for (locationsMap::const_iterator it3 = locations.begin(); it3 != locations.end(); ++it3)
            {
                std::cout << "以下Config::servers_のsecond(VirtualServer*)のlocationsMap locationsのfirst" << std::endl;
                std::cout << it3->first << std::endl; // ここが空行だから複数locationが作れないらしい。->修正済み
                std::cout << std::endl;

                std::cout << "以下(VirtualServer*)のlocationsMap locationsのsecond(Location*)のlocationMap locationSettingの中身" << std::endl;
                locationMap locationSetting = it3->second->locationSetting;
                for (locationMap::const_iterator it4 = locationSetting.begin(); it4 != locationSetting.end(); ++it4)
                {
                    std::cout << it4->first << ":" << it4->second << std::endl;
                } // VirtualServerのlocationSettingを出力->mapだから出力結果はアルファベット順？
                // aaa bbb;などのパターンでは、格納されずWARNIGWARNIG...と出力されてはいるが継続->プログラムを終了させるべき？

            } // VirtualServerのlocationsを出力

            std::cout << "----------------------------------------------" << std::endl;
        }



    }catch(std::runtime_error& e){
        std::cerr << e.what() << std::endl;
    }catch(std::bad_alloc& e){
        std::cerr << e.what() << std::endl;
    }

}
