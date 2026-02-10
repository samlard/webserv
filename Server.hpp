#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include <map>
#include <vector>
#include <poll.h>

// Configuration constants
#define MAX_REQUEST_SIZE 8192        // 8KB max request size
#define CLIENT_TIMEOUT_SECONDS 30    // 30 second timeout for inactive clients
#define POLL_TIMEOUT_MS 1000        // 1 second poll timeout

class Server {
private:
    int listening_fd;                    // Listening socket
    std::map<int, Client> clients;       // Map of fd -> Client
    
    // Helper methods
    void acceptNewConnection();
    void handleClientRead(Client& client);
    void handleClientWrite(Client& client);
    void processRequest(Client& client);
    void closeClient(int fd);
    void cleanupInactiveClients();
    bool isRequestComplete(const Client& client);
    void generateResponse(Client& client, const std::string& request);
    
public:
    Server(int port);
    ~Server();
    
    void run();  // Main event loop
};

#endif // SERVER_HPP
