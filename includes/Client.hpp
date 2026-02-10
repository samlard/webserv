#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Socket.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <string>

// Represents a connected client
class Client {
private:
	Socket _socket;
	HttpRequest _request;
	HttpResponse _response;
	std::string _read_buffer;
	std::string _write_buffer;
	size_t _bytes_sent;
	bool _keep_alive;
	bool _response_ready;

	Client(const Client& other); // Not implemented
	Client& operator=(const Client& other); // Not implemented

public:
	Client(int fd);
	~Client();

	// Read data from socket (returns bytes read, -1 on error)
	int readData();
	
	// Write data to socket (returns bytes written, -1 on error)
	int writeData();
	
	// Check if request is ready for processing
	bool isRequestReady() const;
	
	// Check if response is ready to send
	bool isResponseReady() const;
	
	// Check if all response data has been sent
	bool isResponseSent() const;
	
	// Getters
	int getFd() const;
	HttpRequest& getRequest();
	HttpResponse& getResponse();
	
	// Set response ready to be sent
	void setResponseReady();
	
	// Close connection
	bool shouldClose() const;
};

#endif
