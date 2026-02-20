#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <string.h>
#include <iostream>
#include <map>

class Client {
private:
    int                 _clientFd;        // fd du client
    std::string         _requestBuffer;   // données reçues brutes
    std::string         _responseBuffer;  // réponse à envoyer
    bool                _headersParsed;   // headers déjà traités ?
    size_t              _contentLength;   // taille du body attendue
    std::string         _method;          // GET, POST, DELETE...
    std::string         _uri;             // chemin demandé
    std::string         _httpVersion;     // HTTP/1.1
    std::map<std::string, std::string> _headers;

public:
    Client(int fd);
    ~Client();
    int  getClientFd() const;
    // void appendToRequest(const std::string& data);
    // bool isRequestComplete() const;
    // void buildResponse();
    // const std::string& getResponse() const;
};

#endif