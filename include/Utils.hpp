#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>

namespace Utils {
    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::string toUpper(const std::string& str);
    std::string toLower(const std::string& str);
    std::string intToString(int value);
    int stringToInt(const std::string& str);
    std::string hexToInt(const std::string& hex);
}

#endif
