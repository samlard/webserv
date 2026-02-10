# Poll-Based Non-Blocking HTTP Server Design

## Overview
This document describes the architecture of a single poll()-based event loop for a non-blocking HTTP server that meets C++98 requirements.

## Core Architecture

### 1. Client State Machine

Each client connection has a state that determines what operations to perform:

```
READING_REQUEST   -> Reading HTTP request data
PROCESSING        -> Parsing and preparing response
WRITING_RESPONSE  -> Sending HTTP response data
DONE              -> Response complete, can close or keep-alive
```

State transitions:
- **READING_REQUEST**: 
  - Wait for POLLIN
  - Read available data into read buffer
  - Check if request is complete (see Request Completion Detection)
  - Transition to PROCESSING when complete
  
- **PROCESSING**: 
  - Parse request (no I/O, immediate)
  - Generate response into write buffer
  - Transition to WRITING_RESPONSE
  
- **WRITING_RESPONSE**:
  - Wait for POLLOUT
  - Write available data from write buffer
  - Track bytes written
  - Transition to DONE when all data sent
  
- **DONE**:
  - Close connection or reset for keep-alive
  - Remove from poll set

### 2. POLLIN / POLLOUT Handling

**POLLIN (Data available to read):**
- On listening socket: Accept new connection
- On client socket: Read request data into buffer

**POLLOUT (Socket ready for writing):**
- On client socket: Write response data from buffer

**Key Strategy:**
```cpp
// Only set events we're interested in based on state
if (client.state == READING_REQUEST) {
    pollfd.events = POLLIN;
} else if (client.state == WRITING_RESPONSE) {
    pollfd.events = POLLOUT;
}
```

**Important:** Always check returned `revents` to see what actually happened:
- `POLLIN`: Data available for reading
- `POLLOUT`: Space available for writing
- `POLLERR`, `POLLHUP`, `POLLNVAL`: Error conditions, close connection

### 3. Per-Client Buffer Storage

Each client maintains two buffers:

```cpp
struct Client {
    int fd;
    ClientState state;
    
    // Read buffer for incoming request
    std::string read_buffer;
    size_t read_pos;  // Position in buffer
    
    // Write buffer for outgoing response
    std::string write_buffer;
    size_t write_pos;  // How many bytes written so far
    
    // Optional: timing for timeout detection
    time_t last_activity;
};
```

**Read Buffer Strategy:**
- Append received data to `read_buffer`
- Keep accumulating until request is complete
- No size limit in simple version, but production should have max size

**Write Buffer Strategy:**
- Generate entire response into `write_buffer`
- Track `write_pos` for partial writes
- Continue from `write_pos` on next POLLOUT

### 4. Request Completion Detection

HTTP request is complete when we've received all headers and body:

**For Headers:** Look for "\r\n\r\n" (CRLF CRLF) sequence:
```cpp
if (read_buffer.find("\r\n\r\n") != std::string::npos) {
    // Headers complete
}
```

**For Body (if Content-Length present):**
```cpp
// Parse Content-Length from headers
size_t content_length = parseContentLength(headers);
size_t headers_end = read_buffer.find("\r\n\r\n") + 4;
size_t body_received = read_buffer.size() - headers_end;

if (body_received >= content_length) {
    // Request complete
}
```

**For Chunked Transfer:** More complex, parse chunk sizes and look for "0\r\n\r\n"

**Simple approach for basic server:**
- Assume GET/HEAD requests (no body)
- Complete when headers end found

### 5. Avoiding Infinite Hang

Multiple strategies to prevent hanging:

**A. Timeout Detection:**
```cpp
time_t now = time(NULL);
if (now - client.last_activity > TIMEOUT_SECONDS) {
    // Close connection
}
```

**B. Maximum Request Size:**
```cpp
if (read_buffer.size() > MAX_REQUEST_SIZE) {
    // Send 413 Payload Too Large and close
}
```

**C. poll() Timeout:**
```cpp
// poll() with timeout prevents indefinite blocking
int ready = poll(fds, nfds, POLL_TIMEOUT_MS);
if (ready == 0) {
    // Timeout - check for inactive clients
}
```

**D. Non-Blocking I/O:**
- All sockets set to O_NONBLOCK
- recv()/send() return immediately with EAGAIN/EWOULDBLOCK
- Never blocks waiting for I/O

**E. Error Detection:**
```cpp
if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
    // Connection error, close it
}
```

## Event Loop Pseudocode

```cpp
// Setup
listening_socket = create_listening_socket();
set_nonblocking(listening_socket);
std::vector<pollfd> poll_fds;
std::map<int, Client> clients;

// Add listening socket
poll_fds.push_back({listening_socket, POLLIN, 0});

while (true) {
    // Build poll set
    poll_fds.clear();
    poll_fds.push_back({listening_socket, POLLIN, 0});
    
    for (each client) {
        pollfd pfd;
        pfd.fd = client.fd;
        pfd.events = 0;
        
        if (client.state == READING_REQUEST)
            pfd.events |= POLLIN;
        else if (client.state == WRITING_RESPONSE)
            pfd.events |= POLLOUT;
            
        poll_fds.push_back(pfd);
    }
    
    // Wait for events
    int ready = poll(poll_fds.data(), poll_fds.size(), TIMEOUT_MS);
    
    if (ready < 0) {
        // Error
        continue;
    }
    
    if (ready == 0) {
        // Timeout - check for inactive clients
        cleanup_inactive_clients();
        continue;
    }
    
    // Process ready sockets
    for (each pollfd in poll_fds) {
        if (pollfd.revents == 0)
            continue;
            
        if (pollfd.fd == listening_socket) {
            if (pollfd.revents & POLLIN) {
                accept_new_connection();
            }
        } else {
            Client& client = clients[pollfd.fd];
            
            // Check for errors
            if (pollfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                close_client(client);
                continue;
            }
            
            // Handle based on state
            if (pollfd.revents & POLLIN && client.state == READING_REQUEST) {
                handle_client_read(client);
            }
            
            if (pollfd.revents & POLLOUT && client.state == WRITING_RESPONSE) {
                handle_client_write(client);
            }
        }
    }
}
```

## Key C++98 Considerations

1. **No C++11 features:** Use `std::vector`, `std::map`, `std::string`
2. **poll() instead of epoll:** More portable, works on all platforms
3. **fcntl() for non-blocking:** Use `fcntl(fd, F_SETFL, O_NONBLOCK)`
4. **Manual buffer management:** No smart pointers, use RAII carefully
5. **Error handling:** Check return values of all system calls

## Benefits of This Design

1. **Single poll() call:** All sockets monitored together
2. **No blocking:** Non-blocking I/O + poll with timeout
3. **No busy loop:** Only wakes when events occur or timeout
4. **Scalable:** Can handle many connections efficiently
5. **Simple state machine:** Easy to understand and debug
6. **Proper cleanup:** Timeouts and error handling prevent leaks

## Edge Cases Handled

- **Partial reads:** Accumulate in read_buffer until complete
- **Partial writes:** Track write_pos and continue on next POLLOUT
- **Slow clients:** Timeout mechanism closes inactive connections
- **Connection drops:** POLLHUP detected and cleaned up
- **Large requests:** Optional size limit with 413 response
- **Keep-alive:** Can reset client state after DONE
