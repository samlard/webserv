#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

std::string getMimeType(const std::string& path);
std::string getStatusText(int code);
std::string intToString(int n);
std::string generateDirectoryListing(const std::string& dirPath, const std::string& uri);

#endif
