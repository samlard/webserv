#include "Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <iostream>

Client::Client(int fd) : _socket(fd), _bytes_sent(0), 
	_keep_alive(false), _response_ready(false) {}

Client::~Client() {}

int Client::readData() {
	char buffer[4096];
	ssize_t bytes = recv(_socket.getFd(), buffer, sizeof(buffer), 0);
	
	if (bytes < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 0;
		return -1;
	}
	
	if (bytes == 0)
		return -1; // Connection closed
	
	_read_buffer.append(buffer, bytes);
	_request.parse(_read_buffer);
	_read_buffer.clear();
	
	return bytes;
}

int Client::writeData() {
	if (_write_buffer.empty())
		_write_buffer = _response.getResponseString();
	
	if (_write_buffer.empty())
		return 0;
	
	ssize_t bytes = send(_socket.getFd(), 
		_write_buffer.c_str() + _bytes_sent,
		_write_buffer.size() - _bytes_sent, 0);
	
	if (bytes < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 0;
		return -1;
	}
	
	_bytes_sent += bytes;
	return bytes;
}

bool Client::isRequestReady() const {
	return _request.isComplete();
}

bool Client::isResponseReady() const {
	return _response_ready;
}

bool Client::isResponseSent() const {
	return _response_ready && !_write_buffer.empty() && 
		   _bytes_sent >= _write_buffer.size();
}

int Client::getFd() const {
	return _socket.getFd();
}

HttpRequest& Client::getRequest() {
	return _request;
}

HttpResponse& Client::getResponse() {
	return _response;
}

void Client::setResponseReady() {
	_response_ready = true;
	_bytes_sent = 0;
}

bool Client::shouldClose() const {
	return !_keep_alive || _request.hasError();
}
