#include "../includes/Client.hpp"

Client::Client() : _fd(-1), _serverIndex(-1), _requestComplete(false) {}

Client::Client(int fd, int serverIndex) 
    : _fd(fd), _serverIndex(serverIndex), _requestComplete(false) {}

int Client::getFd() const {
    return _fd;
}

int Client::getServerIndex() const {
    return _serverIndex;
}

const std::string& Client::getRequestBuffer() const {
    return _requestBuffer;
}

bool Client::isRequestComplete() const {
    return _requestComplete;
}

const std::string& Client::getResponse() const {
    return _response;
}

void Client::setServerIndex(int index) {
    _serverIndex = index;
}

void Client::appendToRequest(const std::string& data) {
    _requestBuffer += data;
    
    if (_requestBuffer.find("\r\n\r\n") != std::string::npos) {
        _requestComplete = true;
    }
}

void Client::markRequestComplete() {
    _requestComplete = true;
}

void Client::setResponse(const std::string& response) {
    _response = response;
}

void Client::clear() {
    _requestBuffer.clear();
    _response.clear();
    _requestComplete = false;
}