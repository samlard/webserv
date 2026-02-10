#include "Server.hpp"
#include <iostream>
#include <cerrno>
#include <cstring>
#include <signal.h>

Server::Server() : _running(false) {}

Server::Server(const std::string& config_file) : _running(false) {
	_config.load(config_file);
}

Server::~Server() {
	stop();
}

bool Server::initialize() {
	const std::vector<ServerConfig>& servers = _config.getServers();
	if (servers.empty()) {
		std::cerr << "Error: No server configuration found" << std::endl;
		return false;
	}

	setupListeningSockets();
	if (_listening_sockets.empty()) {
		std::cerr << "Error: No listening sockets created" << std::endl;
		return false;
	}

	return true;
}

void Server::setupListeningSockets() {
	const std::vector<ServerConfig>& servers = _config.getServers();
	
	for (size_t i = 0; i < servers.size(); ++i) {
		const ServerConfig& server = servers[i];
		for (size_t j = 0; j < server.ports.size(); ++j) {
			Socket* sock = new Socket();
			if (sock->createListening(server.ports[j])) {
				_listening_sockets.push_back(sock);
			} else {
				delete sock;
			}
		}
	}
	
	rebuildPollFds();
}

void Server::rebuildPollFds() {
	_pollfds.clear();
	
	// Add listening sockets
	for (size_t i = 0; i < _listening_sockets.size(); ++i) {
		struct pollfd pfd;
		pfd.fd = _listening_sockets[i]->getFd();
		pfd.events = POLLIN;
		pfd.revents = 0;
		_pollfds.push_back(pfd);
	}
	
	// Add client sockets
	for (std::map<int, Client*>::iterator it = _clients.begin();
		 it != _clients.end(); ++it) {
		struct pollfd pfd;
		pfd.fd = it->first;
		pfd.events = POLLIN;
		if (it->second->isResponseReady())
			pfd.events |= POLLOUT;
		pfd.revents = 0;
		_pollfds.push_back(pfd);
	}
}

void Server::run() {
	_running = true;
	std::cout << "Server running..." << std::endl;
	
	while (_running) {
		int ret = poll(&_pollfds[0], _pollfds.size(), 5000);
		
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			std::cerr << "Error: poll() failed: " << strerror(errno) << std::endl;
			break;
		}
		
		if (ret == 0)
			continue; // Timeout
		
		// Process events
		size_t listening_count = _listening_sockets.size();
		for (size_t i = 0; i < _pollfds.size(); ++i) {
			if (_pollfds[i].revents == 0)
				continue;
			
			// Check for errors
			if (_pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
				if (i >= listening_count) {
					closeClient(_pollfds[i].fd);
				}
				continue;
			}
			
			// Listening socket - accept new connection
			if (i < listening_count && (_pollfds[i].revents & POLLIN)) {
				acceptNewConnection(_listening_sockets[i]);
			}
			// Client socket - read/write
			else if (i >= listening_count) {
				if (_pollfds[i].revents & POLLIN) {
					handleClientRead(_pollfds[i].fd);
				}
				if (_pollfds[i].revents & POLLOUT) {
					handleClientWrite(_pollfds[i].fd);
				}
			}
		}
		
		rebuildPollFds();
	}
}

void Server::acceptNewConnection(Socket* listening_socket) {
	int client_fd = listening_socket->acceptConnection();
	if (client_fd < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			std::cerr << "Error: accept() failed" << std::endl;
		return;
	}
	
	Client* client = new Client(client_fd);
	_clients[client_fd] = client;
	std::cout << "New connection on fd " << client_fd << std::endl;
}

void Server::handleClientRead(int fd) {
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;
	
	Client* client = it->second;
	int bytes = client->readData();
	
	if (bytes < 0) {
		closeClient(fd);
		return;
	}
	
	if (client->isRequestReady() && !client->isResponseReady()) {
		processRequest(client);
		client->setResponseReady();
	}
}

void Server::handleClientWrite(int fd) {
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;
	
	Client* client = it->second;
	int bytes = client->writeData();
	
	if (bytes < 0 || client->isResponseSent()) {
		closeClient(fd);
	}
}

void Server::processRequest(Client* client) {
	const ServerConfig& server_config = _config.getServers()[0]; // Simplified
	RequestHandler handler(server_config);
	CgiHandler cgi_handler(server_config);
	
	HttpRequest& request = client->getRequest();
	HttpResponse& response = client->getResponse();
	
	// Check if CGI request
	if (handler.isCgiRequest(request.getUri())) {
		// Remove query string from path for script execution
		std::string uri = request.getUri();
		size_t query_pos = uri.find('?');
		if (query_pos != std::string::npos)
			uri = uri.substr(0, query_pos);
		
		std::string script_path = "./www" + uri;
		cgi_handler.handle(request, response, script_path);
	} else {
		handler.handle(request, response);
	}
}

void Server::closeClient(int fd) {
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it != _clients.end()) {
		std::cout << "Closing connection on fd " << fd << std::endl;
		delete it->second;
		_clients.erase(it);
	}
}

void Server::stop() {
	_running = false;
	
	// Close all clients
	for (std::map<int, Client*>::iterator it = _clients.begin();
		 it != _clients.end(); ++it) {
		delete it->second;
	}
	_clients.clear();
	
	// Close listening sockets
	for (size_t i = 0; i < _listening_sockets.size(); ++i) {
		delete _listening_sockets[i];
	}
	_listening_sockets.clear();
}
