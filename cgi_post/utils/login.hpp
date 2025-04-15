#ifndef LOGIN_HPP
#define LOGIN_HPP

#include <string>
#include <fstream>

std::string createSessionId();
std::string getUserName(std::string requestBody);
std::string getPassword(std::string requestBody);
bool checkLetter(std::string username, std::string password);
std::string getUserSessionId(std::string username);
int searchUser(std::string username, std::string password);
std::string createResponseHeader(std::string resStatus, std::string loginStatus, std::string date);
void registerUserInfo(std::string sessionId, std::string username, std::string password);

#endif