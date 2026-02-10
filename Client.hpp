#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>

// Client connection states
enum ClientState {
    READING_REQUEST,    // Reading HTTP request from client
    PROCESSING,         // Processing request and preparing response
    WRITING_RESPONSE,   // Writing HTTP response to client
    DONE               // Response complete, ready to close or keep-alive
};

// Per-client connection information
struct Client {
    int fd;                      // Socket file descriptor
    ClientState state;           // Current state in the state machine
    
    // Read buffer for incoming HTTP request
    std::string read_buffer;     // Accumulated request data
    
    // Write buffer for outgoing HTTP response
    std::string write_buffer;    // Response to be sent
    size_t write_pos;           // How many bytes already written
    
    // Timing for timeout detection
    time_t last_activity;       // Last time any I/O occurred
    
    // Constructor
    Client(int socket_fd) 
        : fd(socket_fd)
        , state(READING_REQUEST)
        , write_pos(0)
        , last_activity(time(NULL))
    {}
};

#endif // CLIENT_HPP
