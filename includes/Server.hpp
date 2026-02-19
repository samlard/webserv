#ifndef SERVER_HPP
#define SERVER_HPP

#include "Config.hpp"
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
    int _listenSocket;
    std::vector<pollfd> _fds;   // ← Remplace fd_set _masterSet + int _maxFd
    bool _running;

public:
    Server();
    ~Server();
    void shutdown();
    int init(Config &config);
    void run();
};

#endif