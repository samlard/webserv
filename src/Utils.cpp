#include "Utils.hpp"
#include <cctype>
#include <algorithm>

namespace Utils {

std::string trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(str[start]))
        start++;
    while (end > start && std::isspace(str[end - 1]))
        end--;
    
    return str.substr(start, end - start);
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string toUpper(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.length(); i++) {
        result[i] = std::toupper(result[i]);
    }
    return result;
}

std::string toLower(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.length(); i++) {
        result[i] = std::tolower(result[i]);
    }
    return result;
}

std::string intToString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

int stringToInt(const std::string& str) {
    std::istringstream iss(str);
    int value;
    iss >> value;
    return value;
}

std::string hexToInt(const std::string& hex) {
    std::istringstream iss(hex);
    int value;
    iss >> std::hex >> value;
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

}
