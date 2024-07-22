#include "login.hpp"
#include "../../HttpConnection/HttpConnection.hpp"

std::vector<std::string> split(std::string str, char sep) {
   size_t first = 0;
   std::string::size_type last = str.find_first_of(sep);
   std::vector<std::string> result;
    if(last == std::string::npos){
        result.push_back(str);
        return result;
    }
   while (first < str.size()) {
     if(str.substr(first, last - first) != "")
        result.push_back(str.substr(first, last - first));
     first = last + 1;
     last = str.find_first_of(sep, first);
     if (last == std::string::npos) last = str.size();
   }
   return result;
 }

std::string createSessionId(){
    std::time_t t = std::time(NULL);
    struct tm *now = std::localtime(&t);
    char sessionId[20];
    std::strftime(sessionId, sizeof(sessionId), "%Y%m%d%H%M%S", now);
    return std::string(sessionId);
}

std::string getUserName(std::string requestBody){
    size_t idx = requestBody.find("&password=", 0);
    size_t label_len = std::strlen("username=");
    return requestBody.substr(label_len, idx-label_len);
}

std::string getPassword(std::string requestBody){
    size_t idx = requestBody.find("&password=", 0);
    size_t label_len = std::strlen("&password=");
    return requestBody.substr(idx+label_len);
}

bool checkLetter(std::string username, std::string password){
    for(size_t i=0;i<username.length();i++){
        if(!std::isalnum(username[i]))
            return true;
    }
    for(size_t i=0;i<password.length();i++){
        if(!std::isalnum(password[i]))
            return true;
    }
    return false;
}

int searchUser(std::string userName, std::string password){
    std::ifstream ifs("request/UserInfo.txt");
    if(!ifs){
        perror("open failed serchUserName(): request/UserInfo.txt");
        std::exit(1);
    }
    std::string line;
    while(!(ifs).eof()){
        std::getline(ifs, line);
        if(line == "")
            continue;
        std::vector<std::string> vec = split(line, ' ');
        if(vec.size() != 3){
            perror("bad context: ");
            perror(line.c_str());
            perror(userName.c_str());
            perror(password.c_str());
            ifs.close();
            return -1;
        }
        if(vec[1] == userName){
            if(vec[2] == password){
                ifs.close();
                return 2;
            }
            ifs.close();
            return 1;
        }
    }
    ifs.close();
    return 0;
}

std::string createResponseHeader(std::string resStatus, std::string loginStatus, std::string date){
    std::string strOutData;
    (void)loginStatus;
    strOutData = "HTTP/1.1 " + resStatus + "\n";
    strOutData += "Server: webserv42tokyo\n";
    strOutData += "Date: " + date + "\n"; 
    strOutData += "Connection: Keep-Alive\n";
    strOutData += "Content-Length: " + std::to_string(loginStatus.length()) + "\n";
    strOutData += "Content-Type: text/plain; charset=utf-8\n";
    strOutData += "\n";
    strOutData += loginStatus;
    // strOutData += loginStatus + "\n";
    return strOutData;
}


void registerUserInfo(std::string sessionId, std::string username, std::string password){
    std::ofstream ofs("request/UserInfo.txt", std::ios::app);
    if(!ofs){
        perror("registerUserInfo: failed");
        std::exit(1);
    }
    ofs << sessionId + " " + username + " " + password + "\n";
    ofs.close();
}

std::string getUserSessionId(std::string userName){
    std::ifstream ifs("request/UserInfo.txt");
    if(!ifs){
        perror("open failed serchUserName(): request/UserInfo.txt");
        std::exit(1);
    }
    std::string line;
    while(!(ifs).eof()){
        std::getline(ifs, line);
        std::vector<std::string> vec = split(line, ' ');
        if(vec.size() != 3){
            perror("bad context");
            ifs.close();
            exit(1);
        }
        if(vec[1] == userName){
            ifs.close();
            return vec[0];
        }
    }
    ifs.close();
    return "";
}