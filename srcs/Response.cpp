#include "Response.hpp"
#include <sstream>

static const char* getReasonPhrase(int statusCode)
{
    switch (statusCode)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Content Too Large";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

std::string Response::toString() const {

    std::stringstream ss;

    ss << "HTTP/1.1 " << statusCode << " " << getReasonPhrase(statusCode) << "\r\n";

    for (std::map<std::string,std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
        ss << it->first << ": " << it->second << "\r\n";

    ss << "\r\n";
    ss << body;

    return ss.str();
}