#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Config.hpp"
#include <vector>
#include <map>
#include <poll.h>

/**
 * Server - Main server orchestrator
 * 
 * Responsibilities:
 * - Set up listening sockets on configured ports
 * - Run poll() loop for all I/O
 * - Accept new connections
 * - Delegate to Client objects for read/write
 * - Route requests to appropriate handlers
 * - Manage client lifecycle
 * - Handle timeouts and cleanup
 * - Graceful shutdown
 */
class Server {
public:
	Server();
	~Server();

	// Initialize server with configuration
	bool init(const Config& config);
	
	// Main server loop
	void run();
	
	// Graceful shutdown
	void shutdown();

private:
	Config _config;
	std::vector<int> _listenSockets;
	std::map<int, Client*> _clients;
	std::vector<struct pollfd> _pollfds;
	bool _running;
	
	// Socket setup
	bool setupListenSocket(int port);
	void setNonBlocking(int fd);
	
	// Poll loop helpers
	void acceptNewConnection(int listenFd);
	void handleClientRead(int fd);
	void handleClientWrite(int fd);
	void closeClient(int fd);
	void cleanupTimedOutClients();
	
	// Request processing
	void processRequest(Client& client);
	void handleGET(Client& client, const ServerConfig& serverConfig);
	void handlePOST(Client& client, const ServerConfig& serverConfig);
	void handleDELETE(Client& client, const ServerConfig& serverConfig);
	
	// Response helpers
	void serveStaticFile(Client& client, const std::string& filepath);
	void serveDirectory(Client& client, const std::string& dirpath, bool autoindex);
	void sendErrorResponse(Client& client, int statusCode);
	
	// Configuration helpers
	const ServerConfig* findServerConfig(int port) const;
	
	// Poll management
	void addToPoll(int fd, short events);
	void removeFromPoll(int fd);
	void updatePollEvents(int fd, short events);
};

#endif
