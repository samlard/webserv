# Poll-Based HTTP Server - Documentation Index

Welcome to the poll-based non-blocking HTTP server documentation. This project demonstrates a complete implementation of a C++98 HTTP server using a single `poll()` event loop.

## Quick Start

1. **Build**: `make`
2. **Run**: `./webserv 8080`
3. **Test**: `./test_server.sh`

## Documentation Structure

### For Understanding the Design

📘 **[POLL_DESIGN.md](POLL_DESIGN.md)** - Start here!
- Complete architecture explanation
- Client state machine design
- POLLIN/POLLOUT handling strategy
- Per-client buffer management
- Request completion detection
- Avoiding infinite hangs
- Event loop pseudocode

📗 **[KEY_CONCEPTS.md](KEY_CONCEPTS.md)** - Deep dive with code
- Annotated code examples
- Each concept explained in detail
- Common pitfalls and how to avoid them
- Why the design works

### For Using the Server

📕 **[README.md](README.md)** - Overview and quick reference
- Features summary
- Building and running
- Implementation highlights
- File descriptions
- C++98 compliance notes

📙 **[USAGE.md](USAGE.md)** - Practical examples
- Command-line usage
- Testing scenarios
- curl examples
- Load testing
- Monitoring and troubleshooting

### For Security Information

🔒 **[SECURITY.md](SECURITY.md)** - Security analysis
- Security review findings
- Features implemented
- Known limitations
- Best practices

## Source Code Files

### Headers
- **Client.hpp** - Client state and buffer structures
- **Server.hpp** - Server class declaration with event loop

### Implementation
- **Server.cpp** - Complete event loop implementation
  - acceptNewConnection()
  - handleClientRead()
  - handleClientWrite()
  - processRequest()
  - cleanupInactiveClients()
  - run() - main event loop

### Application
- **main.cpp** - Entry point

### Build System
- **Makefile** - Build configuration

### Testing
- **test_server.sh** - Comprehensive test suite
- **test_client.sh** - Simple concurrent test

## Learning Path

### Beginner
1. Read [README.md](README.md) for overview
2. Build and run the server
3. Try examples from [USAGE.md](USAGE.md)
4. Read [POLL_DESIGN.md](POLL_DESIGN.md) sections 1-3

### Intermediate
1. Study [POLL_DESIGN.md](POLL_DESIGN.md) completely
2. Read [KEY_CONCEPTS.md](KEY_CONCEPTS.md) sections 1-6
3. Examine Server.cpp implementation
4. Modify and experiment

### Advanced
1. Complete [KEY_CONCEPTS.md](KEY_CONCEPTS.md)
2. Review [SECURITY.md](SECURITY.md)
3. Understand all edge cases
4. Extend with new features

## Key Questions Answered

### Architecture
- **How to structure client state machine?** → [POLL_DESIGN.md](POLL_DESIGN.md) Section 1
- **How to handle POLLIN/POLLOUT?** → [POLL_DESIGN.md](POLL_DESIGN.md) Section 2
- **How to store per-client buffers?** → [POLL_DESIGN.md](POLL_DESIGN.md) Section 3
- **How to detect request completion?** → [POLL_DESIGN.md](POLL_DESIGN.md) Section 4
- **How to avoid infinite hang?** → [POLL_DESIGN.md](POLL_DESIGN.md) Section 5

### Implementation
- **Non-blocking I/O setup?** → [KEY_CONCEPTS.md](KEY_CONCEPTS.md) Section 7
- **State transitions?** → [KEY_CONCEPTS.md](KEY_CONCEPTS.md) Section 1
- **Partial read handling?** → [KEY_CONCEPTS.md](KEY_CONCEPTS.md) Section 4
- **Partial write handling?** → [KEY_CONCEPTS.md](KEY_CONCEPTS.md) Section 5
- **Error handling?** → [KEY_CONCEPTS.md](KEY_CONCEPTS.md) Section 6

### Usage
- **How to test?** → [USAGE.md](USAGE.md)
- **Performance characteristics?** → [README.md](README.md) + [USAGE.md](USAGE.md)
- **Troubleshooting?** → [USAGE.md](USAGE.md) Troubleshooting section

### Security
- **Is it secure?** → [SECURITY.md](SECURITY.md)
- **What are limitations?** → [SECURITY.md](SECURITY.md) + [README.md](README.md)

## Features Implemented

✅ Single poll() for all sockets  
✅ Simultaneous read/write monitoring  
✅ Non-blocking I/O  
✅ New connection handling  
✅ Partial read handling  
✅ Partial write handling  
✅ Client disconnection handling  
✅ No blocking operations  
✅ No busy loops  
✅ C++98 compliant  
✅ Timeout protection  
✅ Request size limits  
✅ Error handling  

## Project Requirements Met

From the original problem statement:

**Constraints:**
- ✅ One poll() for all client + listening sockets
- ✅ Must monitor read and write simultaneously
- ✅ No read/write without poll readiness
- ✅ Must handle: New connections, Partial reads, Partial writes, Client disconnection
- ✅ No blocking
- ✅ No busy loop
- ✅ C++98 only

**Explanations Provided:**
- ✅ How to structure client state machine
- ✅ How to handle POLLIN / POLLOUT
- ✅ How to store per-client buffers
- ✅ How to detect request completion
- ✅ How to avoid infinite hang

## Additional Resources

### Relevant System Calls
- `poll()` - Wait for events on file descriptors
- `socket()` - Create socket
- `bind()` - Bind socket to address
- `listen()` - Listen for connections
- `accept()` - Accept new connection
- `recv()` - Receive data
- `send()` - Send data
- `fcntl()` - Set non-blocking mode
- `close()` - Close socket

### C++98 STL Used
- `std::vector` - Dynamic array for poll fds
- `std::map` - Client lookup by fd
- `std::string` - Buffer storage
- `std::pair` - Map key-value pairs

### POSIX Headers
- `<poll.h>` - poll() function and structures
- `<sys/socket.h>` - Socket functions
- `<netinet/in.h>` - Internet address structures
- `<fcntl.h>` - File control options
- `<unistd.h>` - POSIX functions
- `<errno.h>` - Error numbers

## Testing

Run the test suite:
```bash
./test_server.sh
```

Tests cover:
- Simple GET requests
- Concurrent connections
- Custom headers
- POST with body
- Sequential requests
- Non-blocking behavior

All tests should pass ✓

## Contributing

This is an educational project demonstrating poll-based event loop design. 

Key principles:
- Keep it simple and readable
- Maintain C++98 compliance
- Follow the documented architecture
- Test thoroughly

## License

Educational/demonstration code.

---

**Start here**: [POLL_DESIGN.md](POLL_DESIGN.md)

**Need help?**: Check [USAGE.md](USAGE.md) troubleshooting section

**Want to learn more?**: Read [KEY_CONCEPTS.md](KEY_CONCEPTS.md)
