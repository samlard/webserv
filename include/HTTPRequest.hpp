#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <vector>

class HTTPRequest {
public:
    HTTPRequest();
    ~HTTPRequest();

    // Parse methods
    bool parse(const std::string& data);
    bool isComplete() const;
    
    // Getters
    std::string getMethod() const;
    std::string getURI() const;
    std::string getVersion() const;
    std::string getHeader(const std::string& key) const;
    std::string getBody() const;
    const std::map<std::string, std::string>& getHeaders() const;
    
    // Chunked encoding handling
    bool isChunked() const;
    std::string getUnchunkedBody() const;

private:
    std::string _method;
    std::string _uri;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    
    bool _headersComplete;
    bool _bodyComplete;
    size_t _contentLength;
    bool _isChunked;
    std::string _rawData;
    
    bool parseRequestLine(const std::string& line);
    bool parseHeaders(const std::string& headerSection);
    bool parseBody();
    std::string unchunkBody(const std::string& chunkedBody) const;
};

#endif
