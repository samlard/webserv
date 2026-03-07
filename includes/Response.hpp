#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class Response {
public:
    int statusCode;
    std::map<std::string, std::string> headers;
    std::string body;

    Response() : statusCode(0) {}

    std::string toString() const;
};

#endif