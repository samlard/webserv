#include "../includes/Server.hpp"
#include "../includes/CGI.hpp"
#include "../includes/Utils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>

#define MAX_EVENTS 100

Server::Server() : _running(false) {
}

Server::~Server() {
	shutdown();
}

bool Server::init(const Config& config) {
	_config = config;
	const std::vector<ServerConfig>& servers = _config.getServers();
	
	if (servers.empty()) {
		std::cerr << "No server configurations found" << std::endl;
		return false;
	}
	
	// Set up listening sockets for all configured ports
	for (size_t i = 0; i < servers.size(); i++) {
		const ServerConfig& server = servers[i];
		for (size_t j = 0; j < server.ports.size(); j++) {
			if (!setupListenSocket(server.ports[j])) {
				std::cerr << "Failed to set up listening socket on port " 
				          << server.ports[j] << std::endl;
				return false;
			}
		}
	}
	
	return true;
}

bool Server::setupListenSocket(int port) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		std::cerr << "Failed to create socket" << std::endl;
		return false;
	}
	
	// Set socket options
	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(fd);
		return false;
	}
	
	// Bind socket
	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		close(fd);
		return false;
	}
	
	// Listen
	if (listen(fd, 128) < 0) {
		close(fd);
		return false;
	}
	
	// Set non-blocking
	setNonBlocking(fd);
	
	_listenSockets.push_back(fd);
	addToPoll(fd, POLLIN);
	
	std::cout << "Listening on port " << port << std::endl;
	return true;
}

void Server::setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::run() {
	_running = true;
	
	while (_running) {
		int ready = poll(&_pollfds[0], _pollfds.size(), 1000);
		
		if (ready < 0) {
			if (errno == EINTR)
				continue;
			std::cerr << "Poll error" << std::endl;
			break;
		}
		
		// Process events
		for (size_t i = 0; i < _pollfds.size() && ready > 0; i++) {
			if (_pollfds[i].revents == 0)
				continue;
			
			ready--;
			int fd = _pollfds[i].fd;
			
			// Check if it's a listening socket
			bool isListenSocket = false;
			for (size_t j = 0; j < _listenSockets.size(); j++) {
				if (fd == _listenSockets[j]) {
					isListenSocket = true;
					break;
				}
			}
			
			if (isListenSocket) {
				acceptNewConnection(fd);
			} else {
				// Client socket
				if (_pollfds[i].revents & POLLIN) {
					handleClientRead(fd);
				} else if (_pollfds[i].revents & POLLOUT) {
					handleClientWrite(fd);
				} else if (_pollfds[i].revents & (POLLERR | POLLHUP)) {
					closeClient(fd);
				}
			}
		}
		
		// Cleanup timed out clients
		cleanupTimedOutClients();
	}
}

void Server::acceptNewConnection(int listenFd) {
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	
	int clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientLen);
	if (clientFd < 0)
		return;
	
	setNonBlocking(clientFd);
	
	Client* client = new Client(clientFd);
	_clients[clientFd] = client;
	
	addToPoll(clientFd, POLLIN);
}

void Server::handleClientRead(int fd) {
	Client* client = _clients[fd];
	if (!client)
		return;
	
	int bytesRead = client->readData();
	
	if (bytesRead <= 0) {
		closeClient(fd);
		return;
	}
	
	// Check if request is complete
	if (client->getState() == Client::PROCESSING) {
		processRequest(*client);
		updatePollEvents(fd, POLLOUT);
	}
}

void Server::handleClientWrite(int fd) {
	Client* client = _clients[fd];
	if (!client)
		return;
	
	int bytesWritten = client->writeData();
	
	if (bytesWritten < 0) {
		closeClient(fd);
		return;
	}
	
	// Check if response is complete
	if (client->getState() == Client::DONE) {
		if (client->shouldKeepAlive()) {
			client->reset();
			updatePollEvents(fd, POLLIN);
		} else {
			closeClient(fd);
		}
	}
}

void Server::closeClient(int fd) {
	Client* client = _clients[fd];
	if (client) {
		delete client;
		_clients.erase(fd);
	}
	removeFromPoll(fd);
}

void Server::cleanupTimedOutClients() {
	time_t currentTime = time(NULL);
	std::vector<int> toClose;
	
	for (std::map<int, Client*>::iterator it = _clients.begin(); 
	     it != _clients.end(); ++it) {
		if (it->second->isTimedOut(currentTime))
			toClose.push_back(it->first);
	}
	
	for (size_t i = 0; i < toClose.size(); i++)
		closeClient(toClose[i]);
}

void Server::processRequest(Client& client) {
	Request& request = client.getRequest();
	
	// Validate request
	if (request.hasError()) {
		sendErrorResponse(client, 400);
		client.setState(Client::WRITING_RESPONSE);
		return;
	}
	
	// Find server configuration (use first server for now)
	const std::vector<ServerConfig>& servers = _config.getServers();
	if (servers.empty()) {
		sendErrorResponse(client, 500);
		client.setState(Client::WRITING_RESPONSE);
		return;
	}
	
	const ServerConfig& serverConfig = servers[0];
	
	// Route to method handler
	const std::string& method = request.getMethod();
	if (method == "GET" || method == "HEAD") {
		handleGET(client, serverConfig);
	} else if (method == "POST") {
		handlePOST(client, serverConfig);
	} else if (method == "DELETE") {
		handleDELETE(client, serverConfig);
	} else {
		sendErrorResponse(client, 405);
	}
	
	client.setState(Client::WRITING_RESPONSE);
}

void Server::handleGET(Client& client, const ServerConfig& serverConfig) {
	Request& request = client.getRequest();
	Response& response = client.getResponse();
	
	std::string uri = Utils::urlDecode(request.getUri());
	
	// Remove query string
	size_t qPos = uri.find('?');
	if (qPos != std::string::npos)
		uri = uri.substr(0, qPos);
	
	// Find matching location
	const ServerConfig::Location* location = serverConfig.findLocation(uri);
	
	// Check for redirect
	if (location && !location->redirect.empty()) {
		response.setStatusCode(301);
		response.setHeader("Location", location->redirect);
		response.setBody("");
		return;
	}
	
	// Determine root directory
	std::string root = serverConfig.root;
	if (location && !location->root.empty())
		root = location->root;
	
	// Build file path
	std::string filepath = Utils::joinPath(root, uri);
	filepath = Utils::normalizePath(filepath);
	
	// Check for CGI
	if (location && CGI::isCGIRequest(uri, *location)) {
		CGI cgi;
		cgi.execute(request, response, *location, filepath);
		return;
	}
	
	// Check if file exists
	if (!Utils::fileExists(filepath)) {
		sendErrorResponse(client, 404);
		return;
	}
	
	// Check if directory
	if (Utils::isDirectory(filepath)) {
		// Try index file
		std::string index = location && !location->index.empty() ? 
		                    location->index : serverConfig.index;
		
		if (!index.empty()) {
			std::string indexPath = Utils::joinPath(filepath, index);
			if (Utils::fileExists(indexPath) && !Utils::isDirectory(indexPath)) {
				serveStaticFile(client, indexPath);
				return;
			}
		}
		
		// Directory listing
		bool autoindex = location ? location->autoindex : false;
		serveDirectory(client, filepath, autoindex);
		return;
	}
	
	// Serve static file
	serveStaticFile(client, filepath);
}

void Server::handlePOST(Client& client, const ServerConfig& serverConfig) {
	Request& request = client.getRequest();
	Response& response = client.getResponse();
	
	std::string uri = Utils::urlDecode(request.getUri());
	
	// Remove query string
	size_t qPos = uri.find('?');
	if (qPos != std::string::npos)
		uri = uri.substr(0, qPos);
	
	// Find matching location
	const ServerConfig::Location* location = serverConfig.findLocation(uri);
	
	// Check body size
	if (request.getBody().length() > serverConfig.clientMaxBodySize) {
		sendErrorResponse(client, 413);
		return;
	}
	
	// Check for CGI
	std::string root = serverConfig.root;
	if (location && !location->root.empty())
		root = location->root;
	
	std::string filepath = Utils::joinPath(root, uri);
	filepath = Utils::normalizePath(filepath);
	
	if (location && CGI::isCGIRequest(uri, *location)) {
		CGI cgi;
		cgi.execute(request, response, *location, filepath);
		return;
	}
	
	// File upload
	if (location && !location->uploadPath.empty()) {
		std::string uploadPath = location->uploadPath;
		std::string filename = "upload_" + Utils::intToString(time(NULL));
		std::string fullPath = Utils::joinPath(uploadPath, filename);
		
		if (Utils::writeFile(fullPath, request.getBody())) {
			response.setStatusCode(201);
			response.setBody("File uploaded successfully");
		} else {
			sendErrorResponse(client, 500);
		}
		return;
	}
	
	sendErrorResponse(client, 405);
}

void Server::handleDELETE(Client& client, const ServerConfig& serverConfig) {
	Request& request = client.getRequest();
	Response& response = client.getResponse();
	
	std::string uri = Utils::urlDecode(request.getUri());
	
	// Remove query string
	size_t qPos = uri.find('?');
	if (qPos != std::string::npos)
		uri = uri.substr(0, qPos);
	
	// Build file path
	std::string root = serverConfig.root;
	std::string filepath = Utils::joinPath(root, uri);
	filepath = Utils::normalizePath(filepath);
	
	// Check if file exists
	if (!Utils::fileExists(filepath)) {
		sendErrorResponse(client, 404);
		return;
	}
	
	// Don't allow deleting directories
	if (Utils::isDirectory(filepath)) {
		sendErrorResponse(client, 403);
		return;
	}
	
	// Delete file
	if (Utils::deleteFile(filepath)) {
		response.setStatusCode(204);
		response.setBody("");
	} else {
		sendErrorResponse(client, 500);
	}
}

void Server::serveStaticFile(Client& client, const std::string& filepath) {
	Response& response = client.getResponse();
	
	if (!Utils::isReadable(filepath)) {
		sendErrorResponse(client, 403);
		return;
	}
	
	response.setBodyFromFile(filepath);
	response.setStatusCode(200);
}

void Server::serveDirectory(Client& client, const std::string& dirpath, bool autoindex) {
	Response& response = client.getResponse();
	
	if (!autoindex) {
		sendErrorResponse(client, 403);
		return;
	}
	
	Request& request = client.getRequest();
	std::string html = Utils::generateDirectoryListing(dirpath, request.getUri());
	
	response.setStatusCode(200);
	response.setHeader("Content-Type", "text/html");
	response.setBody(html);
}

void Server::sendErrorResponse(Client& client, int statusCode) {
	Response& response = client.getResponse();
	
	response.setStatusCode(statusCode);
	response.setHeader("Content-Type", "text/html");
	
	std::string body = "<!DOCTYPE html>\n<html>\n<head>\n<title>" + 
	                   Utils::intToString(statusCode) + " " + 
	                   Response::getStatusText(statusCode) + 
	                   "</title>\n</head>\n<body>\n<h1>" + 
	                   Utils::intToString(statusCode) + " " + 
	                   Response::getStatusText(statusCode) + 
	                   "</h1>\n</body>\n</html>";
	
	response.setBody(body);
}

const ServerConfig* Server::findServerConfig(int port) const {
	const std::vector<ServerConfig>& servers = _config.getServers();
	
	for (size_t i = 0; i < servers.size(); i++) {
		for (size_t j = 0; j < servers[i].ports.size(); j++) {
			if (servers[i].ports[j] == port)
				return &servers[i];
		}
	}
	
	return NULL;
}

void Server::addToPoll(int fd, short events) {
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;
	_pollfds.push_back(pfd);
}

void Server::removeFromPoll(int fd) {
	for (size_t i = 0; i < _pollfds.size(); i++) {
		if (_pollfds[i].fd == fd) {
			_pollfds.erase(_pollfds.begin() + i);
			break;
		}
	}
}

void Server::updatePollEvents(int fd, short events) {
	for (size_t i = 0; i < _pollfds.size(); i++) {
		if (_pollfds[i].fd == fd) {
			_pollfds[i].events = events;
			break;
		}
	}
}

void Server::shutdown() {
	_running = false;
	
	// Close all client connections
	for (std::map<int, Client*>::iterator it = _clients.begin(); 
	     it != _clients.end(); ++it) {
		delete it->second;
	}
	_clients.clear();
	
	// Close listening sockets
	for (size_t i = 0; i < _listenSockets.size(); i++)
		close(_listenSockets[i]);
	_listenSockets.clear();
	
	_pollfds.clear();
}
