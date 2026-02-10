#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <map>
#include <poll.h>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "CGIHandler.hpp"

class Server {
public:
    Server(int port);
    ~Server();

    void run();

private:
    int _port;
    int _serverSocket;
    std::vector<struct pollfd> _pollfds;
    std::map<int, HTTPRequest*> _requests;
    std::map<int, std::string> _responseBuffers;
    std::map<int, CGIHandler*> _cgiHandlers;
    
    void setupServerSocket();
    void acceptConnection();
    void handleClientData(int fd);
    void handleClientWrite(int fd);
    void handleCGIRead(int fd);
    void handleCGIWrite(int fd);
    void handleCGICompletion(int clientFd, CGIHandler* cgi);
    void closeConnection(int fd);
    void sendResponse(int clientFd, const HTTPResponse& response);
    bool isCGIRequest(const HTTPRequest& request);
    std::string getCGIScriptPath(const HTTPRequest& request);
};

#endif
