#include "../includes/Client.hpp"

Client::Client(int fd) : _clientFd(fd) {}

Client::~Client() {}

int Client::getClientFd() const{
    return _clientFd;
}