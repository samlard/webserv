#ifndef SERVER_HPP
#define SERVER_HPP

#include "Config.hpp"
#include "Client.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include <vector>
#include <map>
#include <poll.h>        // ← Remplace <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <iostream>

class Server {
private:
    std::vector<int> _listenSockets;
    std::vector<ServerConfig> _serverConfigs;
    std::vector<pollfd> _fds;
    std::map<int, Client> _clients;    
    bool _running;

public:
    Server();
    ~Server();
    void shutdown();
    int init(Config &config);
    void run();
    void handleClientRead(size_t i);
    void acceptNewClient(int listenSocket);
    int findServerIndex(int listenSocket) const;
    void parseRequest(Client& client);
    std::string buildResponse(Client& client, const std::string& method, const std::string& uri);

};

#endif