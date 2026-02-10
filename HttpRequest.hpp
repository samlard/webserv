#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <vector>

class HttpRequest {
public:
    enum ParseState {
        REQUEST_LINE,
        HEADERS,
        BODY,
        CHUNK_SIZE,
        CHUNK_DATA,
        CHUNK_TRAILER,
        COMPLETE,
        ERROR
    };

    enum ParseResult {
        PARSE_INCOMPLETE,
        PARSE_COMPLETE,
        PARSE_ERROR
    };

    HttpRequest();
    ~HttpRequest();

    // Main parsing function - call with each chunk of data received
    ParseResult parse(const std::string& data);
    
    // Accessors
    std::string getMethod() const;
    std::string getUri() const;
    std::string getVersion() const;
    std::string getHeader(const std::string& name) const;
    std::map<std::string, std::string> getHeaders() const;
    std::string getBody() const;
    int getStatusCode() const;
    std::string getErrorMessage() const;
    ParseState getState() const;
    
    // Configuration
    void setMaxBodySize(size_t size);
    
    // Check request validity
    bool isComplete() const;
    bool hasError() const;

private:
    // Parsing state
    ParseState _state;
    std::string _buffer;
    
    // Request line
    std::string _method;
    std::string _uri;
    std::string _version;
    
    // Headers
    std::map<std::string, std::string> _headers;
    
    // Body
    std::string _body;
    size_t _contentLength;
    bool _isChunked;
    size_t _currentChunkSize;
    size_t _currentChunkRead;
    
    // Configuration
    size_t _maxBodySize;
    
    // Error handling
    int _statusCode;
    std::string _errorMessage;
    
    // Parsing helper functions
    ParseResult parseRequestLine();
    ParseResult parseHeaders();
    ParseResult parseBody();
    ParseResult parseChunkedBody();
    ParseResult parseChunkSize();
    ParseResult parseChunkData();
    ParseResult parseChunkTrailer();
    
    void setError(int statusCode, const std::string& message);
    bool extractLine(std::string& line);
    std::string trim(const std::string& str);
    std::string toLower(const std::string& str) const;
    bool isValidMethod(const std::string& method);
    bool isValidVersion(const std::string& version);
    size_t hexToSize(const std::string& hex);
};

#endif

