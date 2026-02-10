#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>

// Wrapper class for non-blocking sockets
class Socket {
private:
	int _fd;
	int _port;
	bool _is_listening;

	Socket(const Socket& other); // Not implemented
	Socket& operator=(const Socket& other); // Not implemented

public:
	Socket();
	Socket(int fd);
	~Socket();

	// Setup listening socket
	bool createListening(int port);
	
	// Accept new connection
	int acceptConnection();
	
	// Set non-blocking mode
	bool setNonBlocking();
	
	// Getters
	int getFd() const;
	int getPort() const;
	bool isListening() const;
	
	// Close socket
	void close();
};

#endif
