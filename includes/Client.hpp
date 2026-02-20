#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <string.h>
#include <iostream>
#include <map>
#include <sstream>

class Client {
private:
    int _clientFd;
    std::string _requestBuffer;
    std::string _requestBody;
    std::string _responseBuffer;
    bool _headersParsed;
    size_t _contentLength;
    std::string _method;
    std::string _uri;
    std::string _httpVersion;
    std::map<std::string, std::string> _headers;

public:
    Client(int fd) 
        : _clientFd(fd), _headersParsed(false), _contentLength(0) {}
    ~Client() {}

    int getClientFd() const { return _clientFd; }

    void appendToRequest(const std::string& data) {
        _requestBuffer += data;
    }

    bool isRequestComplete() {
        if (!_headersParsed) {
            size_t pos = _requestBuffer.find("\r\n\r\n");
            if (pos != std::string::npos) {
                parseHeaders(_requestBuffer.substr(0, pos));
                _requestBody = _requestBuffer.substr(pos + 4);
                _headersParsed = true;
            } else {
                return false;
            }
        }
        // Si POST, vérifier que le body est complet
        if (_method == "POST" || _method == "DELETE") {
            return _requestBody.size() >= _contentLength;
        }
        return true; // GET n’a pas de body
    }

    void parseHeaders(const std::string& headersStr) {
        std::istringstream stream(headersStr);
        std::string line;
        bool firstLine = true;

        while (std::getline(stream, line)) {
            if (line.back() == '\r')
                line.pop_back();

            if (firstLine) {
                std::istringstream requestLine(line);
                requestLine >> _method >> _uri >> _httpVersion;
                firstLine = false;
            } else {
                size_t delim = line.find(":");
                if (delim != std::string::npos) {
                    std::string key = line.substr(0, delim);
                    std::string value = line.substr(delim + 1);
                    // retirer les espaces en début de valeur
                    size_t start = value.find_first_not_of(" ");
                    if (start != std::string::npos)
                        value = value.substr(start);
                    _headers[key] = value;

                    if (key == "Content-Length")
                        _contentLength = std::stoul(value);
                }
            }
        }
    }

    const std::string& getRequestBody() const { return _requestBody; }
    const std::string& getMethod() const { return _method; }
    const std::string& getUri() const { return _uri; }
    const std::map<std::string, std::string>& getHeaders() const { return _headers; }

    void buildResponse(const std::string& body, const std::string& status = "200 OK") {
        _responseBuffer = "HTTP/1.1 " + status + "\r\n";
        _responseBuffer += "Content-Type: text/html\r\n";
        _responseBuffer += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        _responseBuffer += "\r\n";
        _responseBuffer += body;
    }

    const std::string& getResponse() const { return _responseBuffer; }
};


#endif