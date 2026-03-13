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
#include <sys/wait.h>

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
    Response buildResponse(Client& client);
    Location* matchLocation(const ServerConfig& config, const std::string& uri);
    Response handleGet(const Request& req, const ServerConfig& config, Location* loc);
    Response handlePost(const Request& req, const ServerConfig& config, Location* loc);
    Response handleDelete(const Request& req, const ServerConfig& config, Location* loc);
    bool isCgiRequest(const std::string& path, Location* loc);
    Response executeCgi(const Request& req, const std::string& scriptPath, Location* loc);
    bool isRequestComplete(const std::string& buffer);


};

#endif