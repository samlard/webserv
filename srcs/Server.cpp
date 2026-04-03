#include "../includes/Server.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>

#define BUFFER_SIZE 4096

Server::Server() {
}

Server::~Server() {
}

void Server::shutdown(){
    _running = false;
    for (size_t i = 0; i < _fds.size(); i++)
        close(_fds[i].fd);
    _fds.clear();
    _clients.clear();
    _listenSockets.clear();
}


int Server::findServerIndex(int listenSocket) const 
{
    for (size_t i = 0; i < _listenSockets.size(); i++) {
        if (_listenSockets[i] == listenSocket) {
            return (int)i;
        }
    }
    return -1;
}

int Server::init(Config &config) {

    const std::vector<ServerConfig>& servers = config.getServers();
    
    if (servers.empty()) {
        std::cerr << "Error: No server configuration found" << std::endl;
        return 1;
    }
    
    for (size_t i = 0; i < servers.size(); i++) {
        int port = servers[i].port;
        std::string host = servers[i].host;
        
        if (port == 0) {
            std::cerr << "Error: Server " << i << " has no port" << std::endl;
            continue;
        }
        
        int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (listenSocket < 0) {
            perror("socket");
            for (size_t j = 0; j < _listenSockets.size(); j++) {
                close(_listenSockets[j]);
            }
            return 1;
        }
    
        if (fcntl(listenSocket, F_SETFL, O_NONBLOCK) < 0) {
            perror("fcntl");
            close(listenSocket);
            for (size_t j = 0; j < _listenSockets.size(); j++) {
                close(_listenSockets[j]);
            }
            return 1;
        }
        
        int opt = 1;
        if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt");
            close(listenSocket);
            for (size_t j = 0; j < _listenSockets.size(); j++) {
                close(_listenSockets[j]);
            }
            return 1;
        }
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        
        if (host.empty() || host == "0.0.0.0") {
            addr.sin_addr.s_addr = INADDR_ANY;
            host = "0.0.0.0";
        } else {
            addr.sin_addr.s_addr = inet_addr(host.c_str());
        }
        if (bind(listenSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind");
            close(listenSocket);
            for (size_t j = 0; j < _listenSockets.size(); j++) {
                close(_listenSockets[j]);
            }
            return 1;
        }
        
        if (listen(listenSocket, 128) < 0) {
            perror("listen");
            close(listenSocket);
            for (size_t j = 0; j < _listenSockets.size(); j++) {
                close(_listenSockets[j]);
            }
            return 1;
        }
        
        _listenSockets.push_back(listenSocket);
        _serverConfigs.push_back(servers[i]);
        
        pollfd pfd;
        pfd.fd = listenSocket;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _fds.push_back(pfd);
        std::cout << "  [" << i << "] Listening on " << host << ":" << port 
                  << " (fd=" << listenSocket << ")" << std::endl;
    }
    
    if (_listenSockets.empty()) {
        std::cerr << "Error: No server could be initialized" << std::endl;
        return 1;
    }
    _running = true;
    std::cout << "Server initialized successfully" << std::endl;
    return 0;
}


void Server::run()
{
    while (_running)
    {
        int ret = poll(&_fds[0], _fds.size(), -1);

        if (ret < 0)
        {
            if (!_running)
                break;
            perror("poll");
            break;
        }

        for (int i = (int)_fds.size() - 1; i >= 0; --i)
        {
            int fd = _fds[i].fd;
            short revents = _fds[i].revents;

            if (revents == 0)
                continue;

            bool isListenSocket = false;
            for (size_t j = 0; j < _listenSockets.size(); ++j)
            {
                if (_listenSockets[j] == fd)
                {
                    isListenSocket = true;
                    break;
                }
            }
            if (revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                if (!isListenSocket)
                {
                    close(fd);
                    _clients.erase(fd);
                    _fds.erase(_fds.begin() + i);
                }
                continue;
            }
            if (isListenSocket && (revents & POLLIN))
            {
                acceptNewClient(fd);
                continue;
            }
            if (!isListenSocket && (revents & POLLIN))
            {
                handleClientRead(i);
                continue;
            }

            if (!isListenSocket && (revents & POLLOUT))
            {
                handleClientWrite(i);
                continue;
            }
        }
    }
}



void Server::handleClientRead(size_t i)
{
    int fd = _fds[i].fd;
    char buffer[BUFFER_SIZE];

    ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes <= 0)
    {
        close(fd);
        _clients.erase(fd);
        _fds.erase(_fds.begin() + i);
        return;
    }

    Client& client = _clients[fd];
    client.appendToBuffer(std::string(buffer, bytes));

    if (isRequestComplete(client.getBuffer()))
    {
        client.markRequestComplete();
        parseRequest(client);

        Response res = buildResponse(client);
        client.getResponse() = res;
        client.prepareResponse();

        for (size_t j = 0; j < _fds.size(); ++j)
        {
            if (_fds[j].fd == fd)
            {
                _fds[j].events = POLLOUT;
                break;
            }
        }
    }
}

void Server::handleClientWrite(size_t i)
{
    int fd = _fds[i].fd;

    if (_clients.find(fd) == _clients.end())
    {
        close(fd);
        _fds.erase(_fds.begin() + i);
        return;
    }

    Client& client = _clients[fd];
    const std::string& responseStr = client.getResponseStr();
    size_t remaining = responseStr.size() - client.getSendOffset();

    ssize_t sent = send(fd, responseStr.c_str() + client.getSendOffset(), remaining, 0);

    if (sent <= 0)
    {
        close(fd);
        _clients.erase(fd);
        _fds.erase(_fds.begin() + i);
        return;
    }

    client.advanceSendOffset(static_cast<size_t>(sent));

    if (client.isSendComplete())
    {
        close(fd);
        _clients.erase(fd);
        _fds.erase(_fds.begin() + i);
    }
}

void Server::acceptNewClient(int listenSocket) {
    sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    int clientFd = accept(listenSocket, (sockaddr*)&clientAddr, &addrLen);
    if (clientFd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return;
    }
    
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("fcntl");
        close(clientFd);
        return;
    }
    
    int serverIndex = findServerIndex(listenSocket);
    if (serverIndex == -1) {
        std::cerr << "Error: Could not find server for socket " << listenSocket << std::endl;
        close(clientFd);
        return;
    }
    
    _clients[clientFd] = Client(clientFd, serverIndex);
    
    pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);
    
    std::cout << "New client fd=" << clientFd 
              << " on server [" << serverIndex << "] " 
              << _serverConfigs[serverIndex].host << ":"
              << _serverConfigs[serverIndex].port << std::endl;
}


