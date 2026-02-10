#include "HTTPRequest.hpp"
#include "Utils.hpp"
#include <sstream>
#include <iostream>

HTTPRequest::HTTPRequest() 
    : _headersComplete(false), _bodyComplete(false), 
      _contentLength(0), _isChunked(false) {
}

HTTPRequest::~HTTPRequest() {
}

bool HTTPRequest::parse(const std::string& data) {
    _rawData += data;
    
    // Parse headers if not complete
    if (!_headersComplete) {
        size_t headerEnd = _rawData.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
            return false;
        
        std::string headerSection = _rawData.substr(0, headerEnd);
        std::vector<std::string> lines = Utils::split(headerSection, '\n');
        
        if (lines.empty())
            return false;
        
        // Parse request line
        if (!parseRequestLine(lines[0]))
            return false;
        
        // Parse headers
        std::string headersPart;
        for (size_t i = 1; i < lines.size(); i++) {
            headersPart += lines[i] + "\n";
        }
        if (!parseHeaders(headersPart))
            return false;
        
        _headersComplete = true;
        
        // Check for body
        std::string transferEncoding = getHeader("Transfer-Encoding");
        _isChunked = (Utils::toLower(transferEncoding) == "chunked");
        
        if (!_isChunked) {
            std::string contentLengthStr = getHeader("Content-Length");
            if (!contentLengthStr.empty()) {
                _contentLength = Utils::stringToInt(contentLengthStr);
            }
        }
        
        // Extract body if present
        _body = _rawData.substr(headerEnd + 4);
    }
    
    // Parse body
    return parseBody();
}

bool HTTPRequest::parseRequestLine(const std::string& line) {
    std::string cleanLine = Utils::trim(line);
    std::vector<std::string> parts = Utils::split(cleanLine, ' ');
    
    if (parts.size() != 3)
        return false;
    
    _method = parts[0];
    _uri = parts[1];
    _version = Utils::trim(parts[2]);
    
    return true;
}

bool HTTPRequest::parseHeaders(const std::string& headerSection) {
    std::vector<std::string> lines = Utils::split(headerSection, '\n');
    
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = Utils::trim(lines[i]);
        if (line.empty())
            continue;
        
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;
        
        std::string key = Utils::trim(line.substr(0, colonPos));
        std::string value = Utils::trim(line.substr(colonPos + 1));
        _headers[key] = value;
    }
    
    return true;
}

bool HTTPRequest::parseBody() {
    if (!_headersComplete)
        return false;
    
    if (_isChunked) {
        // For chunked encoding, check if all chunks received
        size_t pos = 0;
        std::string body = _body;
        
        while (pos < body.length()) {
            size_t crlfPos = body.find("\r\n", pos);
            if (crlfPos == std::string::npos)
                return false; // Incomplete chunk size
            
            std::string chunkSizeStr = body.substr(pos, crlfPos - pos);
            int chunkSize = Utils::stringToInt(Utils::hexToInt(chunkSizeStr));
            
            if (chunkSize == 0) {
                _bodyComplete = true;
                return true;
            }
            
            pos = crlfPos + 2;
            if (pos + chunkSize + 2 > body.length())
                return false; // Incomplete chunk data
            
            pos += chunkSize + 2; // Skip chunk data and trailing CRLF
        }
        
        return false;
    } else if (_contentLength > 0) {
        if (_body.length() >= _contentLength) {
            _bodyComplete = true;
            return true;
        }
        return false;
    } else {
        // No body expected
        _bodyComplete = true;
        return true;
    }
}

std::string HTTPRequest::unchunkBody(const std::string& chunkedBody) const {
    std::string result;
    size_t pos = 0;
    
    while (pos < chunkedBody.length()) {
        size_t crlfPos = chunkedBody.find("\r\n", pos);
        if (crlfPos == std::string::npos)
            break;
        
        std::string chunkSizeStr = chunkedBody.substr(pos, crlfPos - pos);
        int chunkSize = Utils::stringToInt(Utils::hexToInt(chunkSizeStr));
        
        if (chunkSize == 0)
            break;
        
        pos = crlfPos + 2;
        result += chunkedBody.substr(pos, chunkSize);
        pos += chunkSize + 2;
    }
    
    return result;
}

bool HTTPRequest::isComplete() const {
    return _headersComplete && _bodyComplete;
}

std::string HTTPRequest::getMethod() const {
    return _method;
}

std::string HTTPRequest::getURI() const {
    return _uri;
}

std::string HTTPRequest::getVersion() const {
    return _version;
}

std::string HTTPRequest::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end())
        return it->second;
    return "";
}

std::string HTTPRequest::getBody() const {
    return _body;
}

const std::map<std::string, std::string>& HTTPRequest::getHeaders() const {
    return _headers;
}

bool HTTPRequest::isChunked() const {
    return _isChunked;
}

std::string HTTPRequest::getUnchunkedBody() const {
    if (_isChunked)
        return unchunkBody(_body);
    return _body;
}
