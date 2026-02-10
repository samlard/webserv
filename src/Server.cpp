#include "Server.hpp"
#include "Utils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <cerrno>

Server::Server(int port) : _port(port), _serverSocket(-1) {
}

Server::~Server() {
    if (_serverSocket != -1)
        close(_serverSocket);
    
    // Clean up requests
    for (std::map<int, HTTPRequest*>::iterator it = _requests.begin();
         it != _requests.end(); ++it) {
        delete it->second;
    }
    
    // Clean up CGI handlers
    for (std::map<int, CGIHandler*>::iterator it = _cgiHandlers.begin();
         it != _cgiHandlers.end(); ++it) {
        delete it->second;
    }
}

void Server::setupServerSocket() {
    // Create socket
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket == -1)
        throw std::runtime_error("Failed to create socket");
    
    // Set socket options
    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        throw std::runtime_error("Failed to set socket options");
    
    // Set non-blocking
    int flags = fcntl(_serverSocket, F_GETFL, 0);
    if (fcntl(_serverSocket, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("Failed to set non-blocking mode");
    
    // Bind
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);
    
    if (bind(_serverSocket, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        throw std::runtime_error("Failed to bind socket");
    
    // Listen
    if (listen(_serverSocket, 10) == -1)
        throw std::runtime_error("Failed to listen on socket");
    
    std::cout << "Server listening on port " << _port << std::endl;
}

void Server::acceptConnection() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSocket = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSocket == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            std::cerr << "Failed to accept connection" << std::endl;
        return;
    }
    
    // Set non-blocking
    int flags = fcntl(clientSocket, F_GETFL, 0);
    if (fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(clientSocket);
        return;
    }
    
    std::cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
    
    // Add to poll list
    struct pollfd pfd;
    pfd.fd = clientSocket;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollfds.push_back(pfd);
    
    // Create request object
    _requests[clientSocket] = new HTTPRequest();
}

void Server::handleClientData(int fd) {
    char buffer[4096];
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer), 0);
    
    if (bytesRead <= 0) {
        if (bytesRead == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            std::cout << "Connection closed by client" << std::endl;
            closeConnection(fd);
        }
        return;
    }
    
    std::string data(buffer, bytesRead);
    HTTPRequest* request = _requests[fd];
    
    if (request->parse(data) && request->isComplete()) {
        std::cout << "Request complete: " << request->getMethod() << " " 
                  << request->getURI() << std::endl;
        
        // Check if this is a CGI request
        if (isCGIRequest(*request)) {
            std::string scriptPath = getCGIScriptPath(*request);
            std::cout << "CGI request detected: " << scriptPath << std::endl;
            
            CGIHandler* cgi = new CGIHandler(*request, scriptPath);
            if (cgi->execute()) {
                _cgiHandlers[fd] = cgi;
                
                // Modify pollfd to monitor CGI pipes
                for (size_t i = 0; i < _pollfds.size(); i++) {
                    if (_pollfds[i].fd == fd) {
                        _pollfds[i].events = 0; // Don't monitor client socket for now
                        break;
                    }
                }
                
                // Add CGI stdout to poll
                int stdoutFd = cgi->getStdoutFd();
                if (stdoutFd != -1) {
                    struct pollfd pfd;
                    pfd.fd = stdoutFd;
                    pfd.events = POLLIN;
                    pfd.revents = 0;
                    _pollfds.push_back(pfd);
                }
                
                // Add CGI stdin to poll if there's data to write
                int stdinFd = cgi->getStdinFd();
                if (stdinFd != -1 && !request->getUnchunkedBody().empty()) {
                    struct pollfd pfd;
                    pfd.fd = stdinFd;
                    pfd.events = POLLOUT;
                    pfd.revents = 0;
                    _pollfds.push_back(pfd);
                } else if (stdinFd != -1) {
                    // No body to write, close stdin immediately
                    cgi->writeToStdin();
                }
            } else {
                delete cgi;
                HTTPResponse response;
                response.setStatus(500, "Internal Server Error");
                response.setBody("CGI execution failed");
                sendResponse(fd, response);
            }
        } else {
            // Regular HTTP request
            HTTPResponse response;
            response.setStatus(200, "OK");
            response.setHeader("Content-Type", "text/plain");
            response.setBody("Hello from WebServ!\n");
            sendResponse(fd, response);
        }
    }
}

void Server::handleClientWrite(int fd) {
    if (_responseBuffers.find(fd) == _responseBuffers.end())
        return;
    
    std::string& buffer = _responseBuffers[fd];
    ssize_t bytesSent = send(fd, buffer.c_str(), buffer.length(), 0);
    
    if (bytesSent == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            closeConnection(fd);
        }
        return;
    }
    
    buffer.erase(0, bytesSent);
    
    if (buffer.empty()) {
        _responseBuffers.erase(fd);
        closeConnection(fd);
    }
}

void Server::handleCGIRead(int fd) {
    // Find which client this CGI belongs to
    int clientFd = -1;
    CGIHandler* cgi = NULL;
    
    for (std::map<int, CGIHandler*>::iterator it = _cgiHandlers.begin();
         it != _cgiHandlers.end(); ++it) {
        if (it->second->getStdoutFd() == fd) {
            clientFd = it->first;
            cgi = it->second;
            break;
        }
    }
    
    if (!cgi)
        return;
    
    cgi->readFromStdout();
    
    // Check if CGI is done
    if (cgi->isDone()) {
        handleCGICompletion(clientFd, cgi);
    }
}

void Server::handleCGIWrite(int fd) {
    // Find which client this CGI belongs to
    CGIHandler* cgi = NULL;
    
    for (std::map<int, CGIHandler*>::iterator it = _cgiHandlers.begin();
         it != _cgiHandlers.end(); ++it) {
        if (it->second->getStdinFd() == fd) {
            cgi = it->second;
            break;
        }
    }
    
    if (!cgi)
        return;
    
    cgi->writeToStdin();
    
    // If stdin is closed, remove from poll
    if (cgi->getStdinFd() == -1) {
        for (size_t i = 0; i < _pollfds.size(); i++) {
            if (_pollfds[i].fd == fd) {
                _pollfds.erase(_pollfds.begin() + i);
                break;
            }
        }
    }
}

void Server::handleCGICompletion(int clientFd, CGIHandler* cgi) {
    std::cout << "CGI execution completed" << std::endl;
    
    // Remove CGI pipes from poll
    for (size_t i = 0; i < _pollfds.size(); ) {
        if (_pollfds[i].fd == cgi->getStdinFd() || 
            _pollfds[i].fd == cgi->getStdoutFd()) {
            _pollfds.erase(_pollfds.begin() + i);
        } else {
            i++;
        }
    }
    
    // Get response and send to client
    HTTPResponse response = cgi->getResponse();
    sendResponse(clientFd, response);
    
    // Clean up
    delete cgi;
    _cgiHandlers.erase(clientFd);
    
    // Re-enable client socket or close it
    for (size_t i = 0; i < _pollfds.size(); i++) {
        if (_pollfds[i].fd == clientFd) {
            _pollfds[i].events = POLLOUT;
            break;
        }
    }
}

void Server::closeConnection(int fd) {
    std::cout << "Closing connection" << std::endl;
    
    // Remove from poll list
    for (size_t i = 0; i < _pollfds.size(); i++) {
        if (_pollfds[i].fd == fd) {
            _pollfds.erase(_pollfds.begin() + i);
            break;
        }
    }
    
    // Clean up request
    if (_requests.find(fd) != _requests.end()) {
        delete _requests[fd];
        _requests.erase(fd);
    }
    
    // Clean up response buffer
    _responseBuffers.erase(fd);
    
    // Clean up CGI handler if exists
    if (_cgiHandlers.find(fd) != _cgiHandlers.end()) {
        delete _cgiHandlers[fd];
        _cgiHandlers.erase(fd);
    }
    
    close(fd);
}

void Server::sendResponse(int clientFd, const HTTPResponse& response) {
    std::string responseStr = response.build();
    _responseBuffers[clientFd] = responseStr;
    
    // Update pollfd to monitor for write
    for (size_t i = 0; i < _pollfds.size(); i++) {
        if (_pollfds[i].fd == clientFd) {
            _pollfds[i].events = POLLOUT;
            break;
        }
    }
}

bool Server::isCGIRequest(const HTTPRequest& request) {
    std::string uri = request.getURI();
    size_t queryPos = uri.find('?');
    std::string path = (queryPos != std::string::npos) ? uri.substr(0, queryPos) : uri;
    
    // Check if path starts with /cgi-bin/
    return (path.find("/cgi-bin/") == 0);
}

std::string Server::getCGIScriptPath(const HTTPRequest& request) {
    std::string uri = request.getURI();
    size_t queryPos = uri.find('?');
    std::string path = (queryPos != std::string::npos) ? uri.substr(0, queryPos) : uri;
    
    // Remove leading /cgi-bin/ and prepend actual path
    if (path.find("/cgi-bin/") == 0) {
        path = path.substr(9); // Remove "/cgi-bin/"
        return "./cgi-bin/" + path;
    }
    
    return "./cgi-bin/" + path;
}

void Server::run() {
    setupServerSocket();
    
    // Add server socket to poll list
    struct pollfd serverPfd;
    serverPfd.fd = _serverSocket;
    serverPfd.events = POLLIN;
    serverPfd.revents = 0;
    _pollfds.push_back(serverPfd);
    
    std::cout << "Server started. Press Ctrl+C to stop." << std::endl;
    
    while (true) {
        int pollCount = poll(&_pollfds[0], _pollfds.size(), -1);
        
        if (pollCount == -1) {
            std::cerr << "Poll error" << std::endl;
            break;
        }
        
        // Process events
        for (size_t i = 0; i < _pollfds.size(); ) {
            struct pollfd& pfd = _pollfds[i];
            
            if (pfd.revents == 0) {
                i++;
                continue;
            }
            
            if (pfd.fd == _serverSocket) {
                // New connection
                if (pfd.revents & POLLIN) {
                    acceptConnection();
                }
                i++;
            } else {
                // Check if this is a CGI pipe
                bool isCGIPipe = false;
                for (std::map<int, CGIHandler*>::iterator it = _cgiHandlers.begin();
                     it != _cgiHandlers.end(); ++it) {
                    if (pfd.fd == it->second->getStdinFd() || 
                        pfd.fd == it->second->getStdoutFd()) {
                        isCGIPipe = true;
                        break;
                    }
                }
                
                if (isCGIPipe) {
                    // CGI pipe event
                    if (pfd.revents & POLLIN) {
                        handleCGIRead(pfd.fd);
                        i++;
                    } else if (pfd.revents & POLLOUT) {
                        handleCGIWrite(pfd.fd);
                        i++;
                    } else if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                        // Error/hangup on CGI pipe
                        
                        // Find which CGI this belongs to FIRST (before closing pipes)
                        int clientFd = -1;
                        CGIHandler* cgi = NULL;
                        for (std::map<int, CGIHandler*>::iterator it = _cgiHandlers.begin();
                             it != _cgiHandlers.end(); ++it) {
                            if (pfd.fd == it->second->getStdinFd() || 
                                pfd.fd == it->second->getStdoutFd()) {
                                clientFd = it->first;
                                cgi = it->second;
                                break;
                            }
                        }
                        
                        // POLLHUP means pipe closed - try to read any remaining data first
                        if (cgi && pfd.fd == cgi->getStdoutFd()) {
                            // Try to read any remaining data
                            cgi->readFromStdout();
                        }
                        
                        // Remove this pipe from poll
                        _pollfds.erase(_pollfds.begin() + i);
                        
                        // Check if CGI is done
                        if (cgi && cgi->isDone()) {
                            handleCGICompletion(clientFd, cgi);
                        }
                        
                        // Don't increment i since we removed an element
                    } else {
                        i++;
                    }
                } else {
                    // Client socket event
                    if (pfd.revents & POLLIN) {
                        handleClientData(pfd.fd);
                    } else if (pfd.revents & POLLOUT) {
                        handleClientWrite(pfd.fd);
                    } else if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                        closeConnection(pfd.fd);
                        continue; // Don't increment i, as we removed the element
                    }
                    i++;
                }
            }
        }
    }
}
