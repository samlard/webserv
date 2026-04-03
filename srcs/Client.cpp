#include "../includes/Client.hpp"

Client::Client() : _fd(-1), _serverIndex(-1), _requestComplete(false), _sendOffset(0) {}

Client::Client(int fd, int serverIndex)
: _fd(fd), _serverIndex(serverIndex), _requestComplete(false), _sendOffset(0) {}

int Client::getFd() const {
    return _fd;
}

int Client::getServerIndex() const {
    return _serverIndex;
}

void Client::setServerIndex(int idx) {
    _serverIndex = idx;
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

void Client::prepareResponse() {
    _responseStr = _response.toString();
    _sendOffset = 0;
}

const std::string& Client::getResponseStr() const {
    return _responseStr;
}

size_t Client::getSendOffset() const {
    return _sendOffset;
}

void Client::advanceSendOffset(size_t n) {
    _sendOffset += n;
}

bool Client::isSendComplete() const {
    return _sendOffset >= _responseStr.size();
}

void Client::clear() {
    _requestBuffer.clear();
    _request.clear();
    _requestComplete = false;
    _responseStr.clear();
    _sendOffset = 0;
}