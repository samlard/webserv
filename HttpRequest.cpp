#include "HttpRequest.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>

HttpRequest::HttpRequest()
    : _state(REQUEST_LINE),
      _contentLength(0),
      _isChunked(false),
      _currentChunkSize(0),
      _currentChunkRead(0),
      _maxBodySize(1048576), // 1MB default
      _statusCode(0)
{
}

HttpRequest::~HttpRequest()
{
}

HttpRequest::ParseResult HttpRequest::parse(const std::string& data)
{
    if (_state == COMPLETE || _state == ERROR) {
        return _state == COMPLETE ? PARSE_COMPLETE : PARSE_ERROR;
    }
    
    _buffer += data;
    
    while (_state != COMPLETE && _state != ERROR) {
        ParseResult result = PARSE_INCOMPLETE;
        
        switch (_state) {
            case REQUEST_LINE:
                result = parseRequestLine();
                break;
            case HEADERS:
                result = parseHeaders();
                break;
            case BODY:
                result = parseBody();
                break;
            case CHUNK_SIZE:
                result = parseChunkSize();
                break;
            case CHUNK_DATA:
                result = parseChunkData();
                break;
            case CHUNK_TRAILER:
                result = parseChunkTrailer();
                break;
            default:
                return PARSE_ERROR;
        }
        
        if (result == PARSE_INCOMPLETE) {
            return PARSE_INCOMPLETE;
        }
        if (result == PARSE_ERROR) {
            return PARSE_ERROR;
        }
    }
    
    return _state == COMPLETE ? PARSE_COMPLETE : PARSE_ERROR;
}

HttpRequest::ParseResult HttpRequest::parseRequestLine()
{
    std::string line;
    if (!extractLine(line)) {
        return PARSE_INCOMPLETE;
    }
    
    // Parse: METHOD URI VERSION
    std::istringstream iss(line);
    iss >> _method >> _uri >> _version;
    
    if (_method.empty() || _uri.empty() || _version.empty()) {
        setError(400, "Bad Request: Malformed request line");
        return PARSE_ERROR;
    }
    
    if (!isValidMethod(_method)) {
        setError(400, "Bad Request: Invalid method");
        return PARSE_ERROR;
    }
    
    if (!isValidVersion(_version)) {
        setError(400, "Bad Request: Invalid HTTP version");
        return PARSE_ERROR;
    }
    
    _state = HEADERS;
    return PARSE_COMPLETE;
}

HttpRequest::ParseResult HttpRequest::parseHeaders()
{
    std::string line;
    
    while (extractLine(line)) {
        // Empty line signals end of headers
        if (line.empty()) {
            // Check for Content-Length or Transfer-Encoding
            std::string contentLengthStr = getHeader("content-length");
            std::string transferEncoding = getHeader("transfer-encoding");
            
            if (!transferEncoding.empty() && toLower(transferEncoding) == "chunked") {
                _isChunked = true;
                _state = CHUNK_SIZE;
            } else if (!contentLengthStr.empty()) {
                std::istringstream iss(contentLengthStr);
                if (!(iss >> _contentLength) || !iss.eof()) {
                    setError(400, "Bad Request: Invalid Content-Length");
                    return PARSE_ERROR;
                }
                
                if (_contentLength > _maxBodySize) {
                    setError(413, "Request Entity Too Large");
                    return PARSE_ERROR;
                }
                
                _state = BODY;
            } else if (_method == "POST") {
                // POST requires Content-Length or chunked
                setError(411, "Length Required");
                return PARSE_ERROR;
            } else {
                // No body expected
                _state = COMPLETE;
                return PARSE_COMPLETE;
            }
            
            return PARSE_COMPLETE;
        }
        
        // Parse header: "Name: Value"
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            setError(400, "Bad Request: Malformed header");
            return PARSE_ERROR;
        }
        
        std::string name = trim(line.substr(0, colonPos));
        std::string value = trim(line.substr(colonPos + 1));
        
        if (name.empty()) {
            setError(400, "Bad Request: Empty header name");
            return PARSE_ERROR;
        }
        
        _headers[toLower(name)] = value;
    }
    
    return PARSE_INCOMPLETE;
}

HttpRequest::ParseResult HttpRequest::parseBody()
{
    if (_buffer.size() >= _contentLength) {
        _body = _buffer.substr(0, _contentLength);
        _buffer.erase(0, _contentLength);
        _state = COMPLETE;
        return PARSE_COMPLETE;
    }
    
    return PARSE_INCOMPLETE;
}

HttpRequest::ParseResult HttpRequest::parseChunkSize()
{
    std::string line;
    if (!extractLine(line)) {
        return PARSE_INCOMPLETE;
    }
    
    // Remove chunk extensions (after ';')
    size_t semicolonPos = line.find(';');
    if (semicolonPos != std::string::npos) {
        line = line.substr(0, semicolonPos);
    }
    
    line = trim(line);
    
    if (line.empty()) {
        setError(400, "Bad Request: Empty chunk size");
        return PARSE_ERROR;
    }
    
    _currentChunkSize = hexToSize(line);
    
    if (_currentChunkSize == static_cast<size_t>(-1)) {
        setError(400, "Bad Request: Invalid chunk size");
        return PARSE_ERROR;
    }
    
    // Check if adding this chunk would exceed max body size
    if (_body.size() + _currentChunkSize > _maxBodySize) {
        setError(413, "Request Entity Too Large");
        return PARSE_ERROR;
    }
    
    if (_currentChunkSize == 0) {
        _state = CHUNK_TRAILER;
    } else {
        _currentChunkRead = 0;
        _state = CHUNK_DATA;
    }
    
    return PARSE_COMPLETE;
}

HttpRequest::ParseResult HttpRequest::parseChunkData()
{
    // Calculate how much data we still need: remaining chunk + CRLF
    size_t remaining = _currentChunkSize - _currentChunkRead;
    
    if (_buffer.size() >= remaining + 2) {
        // We have all remaining chunk data + CRLF
        _body.append(_buffer, 0, remaining);
        
        // Verify CRLF after chunk
        if (_buffer[remaining] != '\r' || _buffer[remaining + 1] != '\n') {
            setError(400, "Bad Request: Missing CRLF after chunk");
            return PARSE_ERROR;
        }
        
        _buffer.erase(0, remaining + 2);
        _state = CHUNK_SIZE;
        return PARSE_COMPLETE;
    } else if (_buffer.size() > 0) {
        // Partial chunk data available
        size_t toRead = std::min(_buffer.size(), remaining);
        _body.append(_buffer, 0, toRead);
        _buffer.erase(0, toRead);
        _currentChunkRead += toRead;
    }
    
    return PARSE_INCOMPLETE;
}

HttpRequest::ParseResult HttpRequest::parseChunkTrailer()
{
    std::string line;
    
    while (extractLine(line)) {
        if (line.empty()) {
            _state = COMPLETE;
            return PARSE_COMPLETE;
        }
        // Ignore trailer headers
    }
    
    return PARSE_INCOMPLETE;
}

void HttpRequest::setError(int statusCode, const std::string& message)
{
    _state = ERROR;
    _statusCode = statusCode;
    _errorMessage = message;
}

bool HttpRequest::extractLine(std::string& line)
{
    size_t pos = _buffer.find("\r\n");
    if (pos == std::string::npos) {
        return false;
    }
    
    line = _buffer.substr(0, pos);
    _buffer.erase(0, pos + 2);
    return true;
}

std::string HttpRequest::trim(const std::string& str)
{
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        start++;
    }
    
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        end--;
    }
    
    return str.substr(start, end - start);
}

std::string HttpRequest::toLower(const std::string& str) const
{
    std::string result = str;
    for (size_t i = 0; i < result.length(); i++) {
        result[i] = std::tolower(static_cast<unsigned char>(result[i]));
    }
    return result;
}

bool HttpRequest::isValidMethod(const std::string& method)
{
    return method == "GET" || method == "POST" || method == "DELETE";
}

bool HttpRequest::isValidVersion(const std::string& version)
{
    return version == "HTTP/1.1" || version == "HTTP/1.0";
}

size_t HttpRequest::hexToSize(const std::string& hex)
{
    size_t result = 0;
    
    // Limit hex string length to prevent overflow
    // size_t max value is at least 2^32-1, represented as 8 hex digits
    // Allow up to 16 hex digits for 64-bit size_t
    if (hex.length() > 16) {
        return static_cast<size_t>(-1);
    }
    
    for (size_t i = 0; i < hex.length(); i++) {
        char c = hex[i];
        int digit = -1;
        
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = 10 + (c - 'a');
        } else if (c >= 'A' && c <= 'F') {
            digit = 10 + (c - 'A');
        } else {
            return static_cast<size_t>(-1);
        }
        
        // Check for overflow before multiplication
        if (result > (static_cast<size_t>(-1) / 16)) {
            return static_cast<size_t>(-1);
        }
        
        result = result * 16 + digit;
    }
    
    return result;
}

// Accessors
std::string HttpRequest::getMethod() const
{
    return _method;
}

std::string HttpRequest::getUri() const
{
    return _uri;
}

std::string HttpRequest::getVersion() const
{
    return _version;
}

std::string HttpRequest::getHeader(const std::string& name) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(toLower(name));
    if (it != _headers.end()) {
        return it->second;
    }
    return "";
}

std::map<std::string, std::string> HttpRequest::getHeaders() const
{
    return _headers;
}

std::string HttpRequest::getBody() const
{
    return _body;
}

int HttpRequest::getStatusCode() const
{
    return _statusCode;
}

std::string HttpRequest::getErrorMessage() const
{
    return _errorMessage;
}

HttpRequest::ParseState HttpRequest::getState() const
{
    return _state;
}

void HttpRequest::setMaxBodySize(size_t size)
{
    _maxBodySize = size;
}

bool HttpRequest::isComplete() const
{
    return _state == COMPLETE;
}

bool HttpRequest::hasError() const
{
    return _state == ERROR;
}
