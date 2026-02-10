#include "HTTPResponse.hpp"
#include "Utils.hpp"

HTTPResponse::HTTPResponse() : _statusCode(200), _statusMessage("OK") {
}

HTTPResponse::~HTTPResponse() {
}

void HTTPResponse::setStatus(int code, const std::string& message) {
    _statusCode = code;
    _statusMessage = message;
}

void HTTPResponse::setHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

void HTTPResponse::setBody(const std::string& body) {
    _body = body;
}

void HTTPResponse::appendBody(const std::string& data) {
    _body += data;
}

std::string HTTPResponse::build() const {
    std::ostringstream oss;
    
    // Status line
    oss << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";
    
    // Headers
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }
    
    // Content-Length if not already set
    if (_headers.find("Content-Length") == _headers.end() && !_body.empty()) {
        oss << "Content-Length: " << _body.length() << "\r\n";
    }
    
    oss << "\r\n";
    
    // Body
    oss << _body;
    
    return oss.str();
}
