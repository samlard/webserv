#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <errno.h>

// Helper function to set socket to non-blocking mode
static void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "fcntl F_GETFL failed" << std::endl;
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "fcntl F_SETFL O_NONBLOCK failed" << std::endl;
    }
}

Server::Server(int port) : listening_fd(-1) {
    // Create listening socket
    listening_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(listening_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(listening_fd);
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }
    
    // Set non-blocking
    setNonBlocking(listening_fd);
    
    // Bind to port
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(listening_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(listening_fd);
        throw std::runtime_error("Failed to bind to port");
    }
    
    // Start listening
    if (listen(listening_fd, SOMAXCONN) < 0) {
        close(listening_fd);
        throw std::runtime_error("Failed to listen");
    }
    
    std::cout << "Server listening on port " << port << std::endl;
}

Server::~Server() {
    // Close all client connections
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
        close(it->first);
    }
    
    // Close listening socket
    if (listening_fd >= 0) {
        close(listening_fd);
    }
}

void Server::acceptNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(listening_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
        }
        return;
    }
    
    // Set client socket to non-blocking
    setNonBlocking(client_fd);
    
    // Create new client and add to map
    clients.insert(std::make_pair(client_fd, Client(client_fd)));
    
    std::cout << "New connection: fd=" << client_fd << std::endl;
}

void Server::handleClientRead(Client& client) {
    char buffer[4096];
    
    ssize_t bytes_read = recv(client.fd, buffer, sizeof(buffer), 0);
    
    if (bytes_read > 0) {
        // Data received - append to read buffer
        client.read_buffer.append(buffer, bytes_read);
        client.last_activity = time(NULL);
        
        std::cout << "Read " << bytes_read << " bytes from fd=" << client.fd << std::endl;
        
        // Check for maximum request size
        if (client.read_buffer.size() > MAX_REQUEST_SIZE) {
            std::cerr << "Request too large from fd=" << client.fd << std::endl;
            // Generate 413 Payload Too Large response
            client.write_buffer = "HTTP/1.1 413 Payload Too Large\r\n"
                                 "Content-Length: 0\r\n"
                                 "Connection: close\r\n\r\n";
            client.write_pos = 0;
            client.state = WRITING_RESPONSE;
            return;
        }
        
        // Check if request is complete
        if (isRequestComplete(client)) {
            client.state = PROCESSING;
            processRequest(client);
        }
    } else if (bytes_read == 0) {
        // Client closed connection
        std::cout << "Client closed connection: fd=" << client.fd << std::endl;
        closeClient(client.fd);
    } else {
        // Error occurred
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Read error from fd=" << client.fd << ": " << strerror(errno) << std::endl;
            closeClient(client.fd);
        }
    }
}

void Server::handleClientWrite(Client& client) {
    if (client.write_pos >= client.write_buffer.size()) {
        // All data sent
        client.state = DONE;
        closeClient(client.fd);
        return;
    }
    
    const char* data = client.write_buffer.c_str() + client.write_pos;
    size_t remaining = client.write_buffer.size() - client.write_pos;
    
    ssize_t bytes_sent = send(client.fd, data, remaining, 0);
    
    if (bytes_sent > 0) {
        // Some data sent
        client.write_pos += bytes_sent;
        client.last_activity = time(NULL);
        
        std::cout << "Wrote " << bytes_sent << " bytes to fd=" << client.fd 
                  << " (" << client.write_pos << "/" << client.write_buffer.size() << ")" << std::endl;
        
        // Check if all data sent
        if (client.write_pos >= client.write_buffer.size()) {
            client.state = DONE;
            closeClient(client.fd);
        }
    } else {
        // Error occurred (bytes_sent < 0) or would block
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Write error to fd=" << client.fd << ": " << strerror(errno) << std::endl;
            closeClient(client.fd);
        }
        // If EAGAIN/EWOULDBLOCK, just return and wait for next POLLOUT
    }
}

bool Server::isRequestComplete(const Client& client) {
    // Look for end of HTTP headers (CRLF CRLF)
    size_t headers_end = client.read_buffer.find("\r\n\r\n");
    if (headers_end == std::string::npos) {
        return false;  // Headers not complete yet
    }
    
    // For simplicity, we'll assume GET/HEAD requests without body
    // In a full implementation, check for Content-Length or Transfer-Encoding
    
    // Check if this is a request with a body
    std::string headers = client.read_buffer.substr(0, headers_end);
    
    // Look for Content-Length header
    size_t content_length_pos = headers.find("Content-Length:");
    if (content_length_pos == std::string::npos) {
        // No Content-Length, assume no body
        return true;
    }
    
    // Parse Content-Length value
    size_t value_start = headers.find(":", content_length_pos) + 1;
    size_t value_end = headers.find("\r\n", value_start);
    std::string length_str = headers.substr(value_start, value_end - value_start);
    
    // Trim whitespace
    size_t first = length_str.find_first_not_of(" \t");
    size_t last = length_str.find_last_not_of(" \t");
    if (first == std::string::npos) {
        return true;  // Invalid Content-Length, treat as no body
    }
    length_str = length_str.substr(first, last - first + 1);
    
    // Convert to number
    std::istringstream iss(length_str);
    size_t content_length = 0;
    iss >> content_length;
    
    // Check if we have received the full body
    size_t body_start = headers_end + 4;  // Skip CRLF CRLF
    size_t body_received = client.read_buffer.size() - body_start;
    
    return body_received >= content_length;
}

void Server::processRequest(Client& client) {
    std::cout << "Processing request from fd=" << client.fd << std::endl;
    
    // Generate response based on request
    generateResponse(client, client.read_buffer);
    
    // Move to writing state
    client.state = WRITING_RESPONSE;
}

void Server::generateResponse(Client& client, const std::string& request) {
    // Parse request line
    size_t first_line_end = request.find("\r\n");
    std::string request_line = request.substr(0, first_line_end);
    
    std::cout << "Request: " << request_line << std::endl;
    
    // Simple response
    std::string body = "<html><body><h1>Hello from Poll-Based Server!</h1>"
                      "<p>Your request was processed successfully.</p>"
                      "<p>This server uses a single poll() call for all sockets.</p>"
                      "</body></html>";
    
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    
    client.write_buffer = response.str();
    client.write_pos = 0;
}

void Server::closeClient(int fd) {
    std::cout << "Closing connection: fd=" << fd << std::endl;
    close(fd);
    clients.erase(fd);
}

void Server::cleanupInactiveClients() {
    time_t now = time(NULL);
    std::vector<int> to_close;
    
    // Find inactive clients
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (now - it->second.last_activity > CLIENT_TIMEOUT_SECONDS) {
            std::cout << "Client timeout: fd=" << it->first << std::endl;
            to_close.push_back(it->first);
        }
    }
    
    // Close them
    for (size_t i = 0; i < to_close.size(); ++i) {
        closeClient(to_close[i]);
    }
}

void Server::run() {
    std::cout << "Starting event loop..." << std::endl;
    
    while (true) {
        // Build poll array
        std::vector<struct pollfd> poll_fds;
        
        // Add listening socket (always interested in POLLIN for new connections)
        struct pollfd listening_pfd;
        listening_pfd.fd = listening_fd;
        listening_pfd.events = POLLIN;
        listening_pfd.revents = 0;
        poll_fds.push_back(listening_pfd);
        
        // Add client sockets
        for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
            struct pollfd client_pfd;
            client_pfd.fd = it->first;
            client_pfd.events = 0;
            client_pfd.revents = 0;
            
            // Set events based on client state
            if (it->second.state == READING_REQUEST) {
                client_pfd.events = POLLIN;
            } else if (it->second.state == WRITING_RESPONSE) {
                client_pfd.events = POLLOUT;
            }
            // PROCESSING state doesn't need any poll events
            
            if (client_pfd.events != 0) {
                poll_fds.push_back(client_pfd);
            }
        }
        
        // Wait for events
        int ready = poll(&poll_fds[0], poll_fds.size(), POLL_TIMEOUT_MS);
        
        if (ready < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted by signal, retry
            }
            std::cerr << "poll() failed: " << strerror(errno) << std::endl;
            break;
        }
        
        if (ready == 0) {
            // Timeout - check for inactive clients
            cleanupInactiveClients();
            continue;
        }
        
        // Process ready sockets
        for (size_t i = 0; i < poll_fds.size(); ++i) {
            if (poll_fds[i].revents == 0) {
                continue;  // No events on this socket
            }
            
            int fd = poll_fds[i].fd;
            short revents = poll_fds[i].revents;
            
            // Check if this is the listening socket
            if (fd == listening_fd) {
                if (revents & POLLIN) {
                    acceptNewConnection();
                }
                continue;
            }
            
            // This is a client socket
            std::map<int, Client>::iterator client_it = clients.find(fd);
            if (client_it == clients.end()) {
                continue;  // Client was closed
            }
            
            Client& client = client_it->second;
            
            // Check for error conditions
            if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
                std::cerr << "Poll error on fd=" << fd << std::endl;
                closeClient(fd);
                continue;
            }
            
            // Handle based on event type and state
            if (revents & POLLIN && client.state == READING_REQUEST) {
                handleClientRead(client);
            }
            
            if (revents & POLLOUT && client.state == WRITING_RESPONSE) {
                handleClientWrite(client);
            }
        }
    }
}
