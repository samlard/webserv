#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include <sstream>

class HTTPResponse {
public:
    HTTPResponse();
    ~HTTPResponse();

    void setStatus(int code, const std::string& message);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    void appendBody(const std::string& data);
    
    std::string build() const;
    
private:
    int _statusCode;
    std::string _statusMessage;
    std::map<std::string, std::string> _headers;
    std::string _body;
};

#endif
