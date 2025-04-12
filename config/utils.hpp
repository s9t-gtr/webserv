#ifndef __UTILS_HPP_
# define __UTILS_HPP_

#include <iostream>
#include <vector>

#define ERROR -1
#define INVALID_DIRECTIVE 0
#define EMPTY 1
#define SERVER_BLOCK 2
#define LOCATION_BLOCK 3
#define DIRECTIVE 4
#define LOCATION_DIRECTIVE 5

bool isEndBraceLine(std::string line);
void cutPrefixSpace(std::string& line);
std::string getDirectiveNameInLine(std::string line);
std::string getDirectiveContentInLine(std::string line, int type);
bool isLocationDirective(std::string line);
int getDirectiveType(std::string line, bool(*isAppropriateDirectiveForBlock)(std::string, std::string[]), std::string AppropriateDirectices[]);
bool isAppropriateDirectiveForBlock(std::string DirectiveName, std::string usableDirectives[]);
std::vector<std::string> split(std::string str, char sep);
#endif