# Poll-Based Event Loop: Key Concepts

This document explains the core concepts of the poll-based event loop with annotated code examples.

## 1. Client State Machine

```cpp
enum ClientState {
    READING_REQUEST,    // Waiting for POLLIN - read request
    PROCESSING,         // CPU-bound - parse and generate response
    WRITING_RESPONSE,   // Waiting for POLLOUT - send response
    DONE               // Complete - close connection
};
```

**State Transitions:**
```
READING_REQUEST → PROCESSING     (when request complete)
PROCESSING → WRITING_RESPONSE    (response generated)
WRITING_RESPONSE → DONE          (all data sent)
DONE → closed                     (connection cleanup)
```

## 2. Setting Poll Events Based on State

```cpp
// Only monitor events we need based on client state
for (each client) {
    struct pollfd pfd;
    pfd.fd = client.fd;
    pfd.events = 0;  // Start with no events
    
    if (client.state == READING_REQUEST) {
        // Want to know when data is available to read
        pfd.events = POLLIN;
    } else if (client.state == WRITING_RESPONSE) {
        // Want to know when socket is ready for writing
        pfd.events = POLLOUT;
    }
    // PROCESSING state doesn't wait for I/O, so no events
    
    if (pfd.events != 0) {
        poll_fds.push_back(pfd);
    }
}
```

**Key Point:** Only set events you're interested in. This prevents unnecessary wake-ups.

## 3. The Poll Call

```cpp
int ready = poll(&poll_fds[0], poll_fds.size(), POLL_TIMEOUT_MS);

// Returns:
// -1 = error
//  0 = timeout (no events in POLL_TIMEOUT_MS)
// >0 = number of file descriptors with events
```

**What poll() does:**
1. Blocks until at least one fd has an event OR timeout occurs
2. Updates the `revents` field of each pollfd structure
3. Returns number of ready file descriptors

**Why timeout is important:**
- Prevents infinite blocking
- Allows periodic cleanup (inactive clients)
- Opportunity to handle signals or shutdown

## 4. Handling POLLIN (Data Available)

```cpp
if (revents & POLLIN) {
    if (fd == listening_socket) {
        // New connection waiting
        acceptNewConnection();
    } else {
        // Data available on client socket
        char buffer[4096];
        ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
        
        if (n > 0) {
            // Got data - append to read buffer
            client.read_buffer.append(buffer, n);
            
            // Check if request is complete
            if (isRequestComplete(client)) {
                client.state = PROCESSING;
                processRequest(client);
            }
        } else if (n == 0) {
            // Client closed connection gracefully
            closeClient(fd);
        } else {
            // Error (unless EAGAIN/EWOULDBLOCK)
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                closeClient(fd);
            }
        }
    }
}
```

**Key Points:**
- recv() with non-blocking socket returns immediately
- EAGAIN/EWOULDBLOCK means no data available (try again later)
- Accumulate partial data in buffer
- Process only when complete

## 5. Handling POLLOUT (Socket Ready for Writing)

```cpp
if (revents & POLLOUT) {
    const char* data = client.write_buffer.c_str() + client.write_pos;
    size_t remaining = client.write_buffer.size() - client.write_pos;
    
    ssize_t n = send(fd, data, remaining, 0);
    
    if (n > 0) {
        // Sent some data
        client.write_pos += n;
        
        if (client.write_pos >= client.write_buffer.size()) {
            // All data sent!
            client.state = DONE;
            closeClient(fd);
        }
        // Otherwise wait for next POLLOUT to send more
    } else {
        // Error (unless EAGAIN/EWOULDBLOCK)
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            closeClient(fd);
        }
    }
}
```

**Key Points:**
- POLLOUT means kernel buffer has space
- send() may not send all data at once
- Track position for partial writes
- Continue on next POLLOUT event

## 6. Handling Errors

```cpp
if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
    // Something went wrong
    // POLLERR: Error condition
    // POLLHUP: Hang up (peer closed)
    // POLLNVAL: Invalid fd (shouldn't happen)
    closeClient(fd);
}
```

## 7. Non-Blocking Socket Setup

```cpp
void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```

**Effect:**
- recv() returns immediately if no data: errno = EAGAIN
- send() returns immediately if buffer full: errno = EWOULDBLOCK
- accept() returns immediately if no connection: errno = EAGAIN

**Without non-blocking:** recv() would block, hanging the entire event loop!

## 8. Request Completion Detection

```cpp
bool isRequestComplete(const Client& client) {
    // Look for end of headers
    size_t pos = client.read_buffer.find("\r\n\r\n");
    if (pos == std::string::npos) {
        return false;  // Headers not complete
    }
    
    // Check if body expected
    if (hasContentLength(client)) {
        size_t body_start = pos + 4;
        size_t body_received = client.read_buffer.size() - body_start;
        size_t content_length = parseContentLength(client);
        return body_received >= content_length;
    }
    
    return true;  // No body expected
}
```

**HTTP Request Structure:**
```
GET / HTTP/1.1\r\n
Host: localhost\r\n
Content-Length: 5\r\n
\r\n
HELLO
```

## 9. Timeout Detection

```cpp
void cleanupInactiveClients() {
    time_t now = time(NULL);
    
    for (each client) {
        if (now - client.last_activity > CLIENT_TIMEOUT_SECONDS) {
            closeClient(client.fd);
        }
    }
}
```

**Update activity timestamp:**
```cpp
// On successful I/O
client.last_activity = time(NULL);
```

## 10. The Complete Event Loop

```cpp
while (true) {
    // Step 1: Build poll array
    vector<pollfd> poll_fds;
    poll_fds.push_back({listening_fd, POLLIN, 0});
    
    for (each client) {
        pollfd pfd = {client.fd, 0, 0};
        if (client.state == READING_REQUEST) pfd.events = POLLIN;
        if (client.state == WRITING_RESPONSE) pfd.events = POLLOUT;
        if (pfd.events) poll_fds.push_back(pfd);
    }
    
    // Step 2: Wait for events
    int ready = poll(&poll_fds[0], poll_fds.size(), TIMEOUT_MS);
    
    if (ready == 0) {
        cleanupInactiveClients();
        continue;
    }
    
    // Step 3: Process ready sockets
    for (each pollfd) {
        if (revents == 0) continue;
        
        if (fd == listening_fd && revents & POLLIN) {
            acceptNewConnection();
        } else if (revents & POLLIN) {
            handleClientRead(client);
        } else if (revents & POLLOUT) {
            handleClientWrite(client);
        } else if (revents & (POLLERR | POLLHUP)) {
            closeClient(client);
        }
    }
}
```

## Why This Design Works

### No Blocking
- All sockets are non-blocking
- poll() has timeout
- No indefinite waits

### No Busy Loop
- poll() blocks until events or timeout
- CPU sleeps when idle
- Efficient resource usage

### Handles Partial I/O
- Read buffer accumulates data
- Write position tracks progress
- State machine manages flow

### Scalable
- Single thread handles many connections
- O(n) where n is number of sockets
- No per-connection threads

### Robust
- Timeouts prevent resource leaks
- Error handling at every level
- Graceful connection closure

## Common Pitfalls to Avoid

### 1. Forgetting Non-Blocking
```cpp
// BAD: Blocking socket in event loop
recv(fd, buf, size, 0);  // Could block forever!

// GOOD: Set non-blocking first
setNonBlocking(fd);
recv(fd, buf, size, 0);  // Returns immediately
```

### 2. Not Checking EAGAIN
```cpp
// BAD: Treating EAGAIN as error
if (recv(fd, buf, size, 0) < 0) {
    close(fd);  // Wrong! EAGAIN is not an error
}

// GOOD: Check errno
if (recv(fd, buf, size, 0) < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
        close(fd);  // Real error
    }
}
```

### 3. Wrong Events
```cpp
// BAD: Always monitoring POLLOUT
pollfd.events = POLLIN | POLLOUT;  // Wastes CPU!

// GOOD: Only when needed
if (client.state == WRITING_RESPONSE) {
    pollfd.events = POLLOUT;
}
```

### 4. Forgetting Partial Writes
```cpp
// BAD: Assuming send() sends everything
send(fd, response.c_str(), response.size(), 0);
close(fd);  // Might not have sent all data!

// GOOD: Track position
client.write_pos += bytes_sent;
if (client.write_pos >= client.write_buffer.size()) {
    close(fd);
}
```

## Summary

The poll-based event loop provides:
- **Efficiency**: CPU sleeps when idle
- **Scalability**: Many connections, one thread
- **Robustness**: Timeouts and error handling
- **Simplicity**: Clear state machine

All while maintaining:
- Non-blocking operation
- No busy loops
- Proper handling of partial I/O
- Clean resource management
