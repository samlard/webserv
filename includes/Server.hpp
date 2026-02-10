#ifndef SERVER_HPP
#define SERVER_HPP

#include "Config.hpp"
#include "Socket.hpp"
#include "Client.hpp"
#include "RequestHandler.hpp"
#include "CgiHandler.hpp"
#include <vector>
#include <map>
#include <poll.h>

// Main server class managing poll() event loop
class Server {
private:
	Config _config;
	std::vector<Socket*> _listening_sockets;
	std::map<int, Client*> _clients;
	std::vector<struct pollfd> _pollfds;
	bool _running;

	void setupListeningSockets();
	void acceptNewConnection(Socket* listening_socket);
	void handleClientRead(int fd);
	void handleClientWrite(int fd);
	void closeClient(int fd);
	void rebuildPollFds();
	void processRequest(Client* client);

	Server(const Server& other); // Not implemented
	Server& operator=(const Server& other); // Not implemented

public:
	Server();
	Server(const std::string& config_file);
	~Server();

	bool initialize();
	void run();
	void stop();
};

#endif
