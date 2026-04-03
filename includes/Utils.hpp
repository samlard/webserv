#ifndef UTILS_HPP
#define UTILS_HPP

#include "Response.hpp"
#include "Config.hpp"
#include <dirent.h> 

void finalizeResponseHeaders(Response& res);
bool isMethodAllowed(const Location* loc, const std::string& method);
std::string stripUriSuffix(const std::string& uri);
std::string buildPathFromLocation(const std::string& uriPath, const std::string& root, const Location* loc);
std::string generateAutoindexBody(const std::string& uri, const std::string& fsPath);
std::string getMimeType(const std::string& path);


#endif