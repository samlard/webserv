#include "Client.hpp"

Client::Client() : _fd(-1), _serverIndex(-1), _requestComplete(false) {}

Client::Client(int fd, int serverIndex)
: _fd(fd), _serverIndex(serverIndex), _requestComplete(false) {}

int Client::getFd() const {
    return _fd;
}

int Client::getServerIndex() const {
    return _serverIndex;
}

std::string& Client::getBuffer() {
    return _requestBuffer;
}

bool Client::isRequestComplete() const {
    return _requestComplete;
}

Request& Client::getRequest() {
    return _request;
}

Response& Client::getResponse() {
    return _response;
}

void Client::appendToBuffer(const std::string& data) {
    _requestBuffer += data;
}

void Client::markRequestComplete() {
    _requestComplete = true;
}

void Client::clear() {

    _requestBuffer.clear();
    _request.clear();
    _requestComplete = false;
}