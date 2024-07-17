#include <iostream>
#include <string>
#include "../HttpConnection/HttpConnection.hpp"

// レスポンスヘッダーに含まれるGMT時刻を取得する関数
static std::string getGmtDate()
{
    std::time_t now = std::time(nullptr);
    std::tm* tm_now = std::gmtime(&now);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm_now);
    
    return (std::string(buffer));
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

    file << request_body + "____HELLO";
    file.close();

    return (timestamp);
}

// ファイルをアップロードしてからレスポンスをメインプロセスに返す
int main(int argc, char** argv)
{
    if(argc != 3)
        exit(1);
    // execuveの引数から必要な情報を取得
    std::string upload_dir = argv[1];
    std::string request_body = argv[2];
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
