#include <iostream>
#include <string>
#include "../HttpConnection/HttpConnection.hpp"
#include "utils/login.hpp"

// レスポンスヘッダーに含まれるGMT時刻を取得する関数
static std::string getGmtDate()
{
    std::time_t now = std::time(nullptr);
    std::tm* tm_now = std::gmtime(&now);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm_now);
    
    return (std::string(buffer));
}

void loginProcessing(std::string request_body){
    std::string username = getUserName(request_body);
    std::string password = getPassword(request_body);
    if(username == "" || password == ""){
        std::string strOutData = createResponseHeader("200 OK", "Please enter your username and password", getGmtDate());
        std::cout << strOutData.c_str();
        return ;
    }
    if(checkLetter(username, password) == true){
        std::string strOutData = createResponseHeader("200 OK", "Only alphabets or numbers can be used", getGmtDate());
        std::cout << strOutData.c_str();
        return ;
    }
    int match = searchUser(username, password);
    if(match == -1)
        perror("storange: loginProcessing()");
    if(match == 1){
        std::string strOutData = createResponseHeader("200 OK", "This username is in use by another user", getGmtDate());
        std::cout << strOutData.c_str();
        return ;
    }
    std::string sessionId;
    if(match == 2){
        sessionId = getUserSessionId(username);
    }else{
        sessionId = createSessionId();
        registerUserInfo(sessionId, username, password);
    }
    std::string strOutData;
    strOutData = "HTTP/1.1 303 See Other\n";
    strOutData += "Connection: Keep-Alive\n";
    strOutData += "Content-Length: "+ std::to_string(std::strlen("Login successful")) + "\n";
    strOutData += "Date: " + getGmtDate() + "\n";
    strOutData += "Server: webserv/1.0.0\n";
    strOutData += "Location: /user.html\n";
    strOutData += "Set-Cookie: SessionID="+sessionId + "; Path=/; Max-Age=3600\n";
    strOutData += "\n";
    strOutData += "Login successful";

    std::cout << strOutData;
    return ;
}

// ファイル名をタイムスタンプにしてアップロードする
static std::string uploadFile(std::string upload_dir, std::string request_body)
{
    // timestamp文字列の作成
    std::time_t t = std::time(NULL);
    struct tm *now = std::localtime(&t);
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%H%M%S", now);

    std::string file_path = upload_dir + timestamp;
    std::ofstream file(file_path.c_str());
    if (!file.is_open())
        exit(1);
    file << request_body + "__HELLO!";
    file.close();

    return (timestamp);
}

// ファイルをアップロードしてからレスポンスをメインプロセスに返す
int main(int argc, char** argv)
{
    if(argc < 3)
        exit(1);
    // execuveの引数から必要な情報を取得
    std::string upload_dir = argv[1];//NOTICE: ひつようなくね
    std::string request_body = argv[2];
    if(request_body.substr(0, 9) == "username="){//login処理
        loginProcessing(request_body);
        return 0;
    }
    // if(argc == 4 && std::string(argv[3]).substr(0, std::strlen("Cookie:")) == "Cookie:"){//user.html作成
    //     createUserPage(std::string(argv[3]));
    // }
    //login処理でないならupload処理
    if(request_body.find("message=", 0) == 0)
        request_body = request_body.substr(8);
    std::string timestamp = uploadFile(upload_dir, request_body);
    upload_dir = "/" + upload_dir;
    std::string strOutData;

    strOutData = "HTTP/1.1 303 See Other\n";
    strOutData += "Connection: Keep-Alive\n";
    strOutData += "Content-Length: 0\n";
    strOutData += "Date: " + getGmtDate() + "\n";
    strOutData += "Server: webserv/1.0.0\n";
    strOutData += "Location: " + upload_dir + "\n";
    strOutData += "\n";

    std::cout << strOutData.c_str();
}
