#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Request.hpp"
#include "Response.hpp"
#include <string>
#include <ctime>

/**
 * Client - Manages a single client connection
 * 
 * Responsibilities:
 * - Hold client socket file descriptor
 * - Manage client state (reading request, writing response)
 * - Buffer incoming data
 * - Track bytes sent/received
 * - Handle partial reads/writes in non-blocking mode
 * - Timeout detection
 */
class Client {
public:
	enum State {
		READING_REQUEST,
		PROCESSING,
		WRITING_RESPONSE,
		DONE,
		ERROR
	};

	Client(int fd);
	~Client();

	// I/O operations (returns bytes read/written, -1 on error)
	int readData();
	int writeData();
	
	// State management
	State getState() const;
	void setState(State state);
	
	// Request/Response access
	Request& getRequest();
	Response& getResponse();
	
	// Socket access
	int getFd() const;
	
	// Timeout check (returns true if client should be closed)
	bool isTimedOut(time_t currentTime) const;
	void updateLastActivity();
	
	// Check if keep-alive
	bool shouldKeepAlive() const;
	
	// Reset for keep-alive
	void reset();

private:
	int _fd;
	State _state;
	Request _request;
	Response _response;
	
	// Write state for partial writes
	std::string _responseBuffer;
	size_t _bytesWritten;
	
	// Timeout management
	time_t _lastActivity;
	
	// Private helpers
	void updateState();
};

#endif
