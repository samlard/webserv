#include "Response.hpp"
#include <sstream>

std::string Response::toString() const {

    std::stringstream ss;

    ss << "HTTP/1.1 " << statusCode << " OK\r\n";

    for (std::map<std::string,std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
        ss << it->first << ": " << it->second << "\r\n";

    ss << "\r\n";
    ss << body;

    return ss.str();
}