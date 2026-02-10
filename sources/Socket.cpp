#include "Socket.hpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>

Socket::Socket() : _fd(-1), _port(0), _is_listening(false) {}

Socket::Socket(int fd) : _fd(fd), _port(0), _is_listening(false) {
	setNonBlocking();
}

Socket::~Socket() {
	close();
}

bool Socket::createListening(int port) {
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0) {
		std::cerr << "Error: socket() failed" << std::endl;
		return false;
	}

	int opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		std::cerr << "Error: setsockopt() failed" << std::endl;
		::close(_fd);
		_fd = -1;
		return false;
	}

	if (!setNonBlocking()) {
		::close(_fd);
		_fd = -1;
		return false;
	}

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		std::cerr << "Error: bind() failed on port " << port << std::endl;
		::close(_fd);
		_fd = -1;
		return false;
	}

	if (listen(_fd, 128) < 0) {
		std::cerr << "Error: listen() failed" << std::endl;
		::close(_fd);
		_fd = -1;
		return false;
	}

	_port = port;
	_is_listening = true;
	std::cout << "Listening on port " << port << std::endl;
	return true;
}

int Socket::acceptConnection() {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(_fd, (struct sockaddr*)&client_addr, &client_len);
	return client_fd;
}

bool Socket::setNonBlocking() {
	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags < 0)
		return false;
	return fcntl(_fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

int Socket::getFd() const {
	return _fd;
}

int Socket::getPort() const {
	return _port;
}

bool Socket::isListening() const {
	return _is_listening;
}

void Socket::close() {
	if (_fd >= 0) {
		::close(_fd);
		_fd = -1;
	}
}
