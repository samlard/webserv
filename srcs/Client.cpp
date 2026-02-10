#include "../includes/Client.hpp"
#include <unistd.h>
#include <ctime>

#define BUFFER_SIZE 8192
#define CLIENT_TIMEOUT 60

Client::Client(int fd) 
	: _fd(fd), _state(READING_REQUEST), _bytesWritten(0), _lastActivity(time(NULL)) {
}

Client::~Client() {
	if (_fd >= 0)
		close(_fd);
}

int Client::readData() {
	char buffer[BUFFER_SIZE];
	int bytesRead = read(_fd, buffer, BUFFER_SIZE);
	
	if (bytesRead <= 0)
		return bytesRead;
	
	_request.parseData(std::string(buffer, bytesRead));
	updateLastActivity();
	updateState();
	
	return bytesRead;
}

int Client::writeData() {
	if (_responseBuffer.empty())
		_responseBuffer = _response.toString();
	
	if (_bytesWritten >= _responseBuffer.length())
		return 0;
	
	int bytesToWrite = _responseBuffer.length() - _bytesWritten;
	if (bytesToWrite > BUFFER_SIZE)
		bytesToWrite = BUFFER_SIZE;
	
	int bytesWritten = write(_fd, 
		_responseBuffer.c_str() + _bytesWritten, 
		bytesToWrite);
	
	if (bytesWritten > 0) {
		_bytesWritten += bytesWritten;
		updateLastActivity();
		
		if (_bytesWritten >= _responseBuffer.length())
			_state = DONE;
	}
	
	return bytesWritten;
}

void Client::updateState() {
	if (_request.hasError()) {
		_state = ERROR;
	} else if (_request.isComplete() && _state == READING_REQUEST) {
		_state = PROCESSING;
	}
}

Client::State Client::getState() const {
	return _state;
}

void Client::setState(State state) {
	_state = state;
}

Request& Client::getRequest() {
	return _request;
}

Response& Client::getResponse() {
	return _response;
}

int Client::getFd() const {
	return _fd;
}

bool Client::isTimedOut(time_t currentTime) const {
	return (currentTime - _lastActivity) > CLIENT_TIMEOUT;
}

void Client::updateLastActivity() {
	_lastActivity = time(NULL);
}

bool Client::shouldKeepAlive() const {
	const std::map<std::string, std::string>& headers = _request.getHeaders();
	std::map<std::string, std::string>::const_iterator it = headers.find("Connection");
	
	if (it != headers.end()) {
		std::string value = it->second;
		// Convert to lowercase for comparison
		for (size_t i = 0; i < value.length(); i++)
			value[i] = std::tolower(value[i]);
		return value == "keep-alive";
	}
	
	// Default for HTTP/1.1 is keep-alive
	return _request.getHttpVersion() == "HTTP/1.1";
}

void Client::reset() {
	_request.reset();
	_response.reset();
	_responseBuffer.clear();
	_bytesWritten = 0;
	_state = READING_REQUEST;
	updateLastActivity();
}
