# Poll-Based Non-Blocking HTTP Server

This is a C++98 implementation of a non-blocking HTTP server using a single `poll()` event loop.

## Features

- **Single poll() call** for all client and listening sockets
- **Non-blocking I/O** on all sockets
- **Simultaneous monitoring** of read and write readiness
- **Proper handling** of:
  - New connections
  - Partial reads
  - Partial writes
  - Client disconnection
  - Timeouts
  - Request size limits
- **No busy loops** - efficient event-driven architecture
- **C++98 compliant** - no C++11 features used

## Architecture

See [POLL_DESIGN.md](POLL_DESIGN.md) for detailed design documentation.

### Client State Machine

Each client connection follows a state machine:

1. **READING_REQUEST**: Reading HTTP request (wait for POLLIN)
2. **PROCESSING**: Parsing request and generating response (immediate)
3. **WRITING_RESPONSE**: Sending HTTP response (wait for POLLOUT)
4. **DONE**: Complete, close connection

### Event Loop

The event loop:
1. Builds array of pollfd structures for listening socket + all clients
2. Calls `poll()` with timeout
3. Processes ready sockets:
   - POLLIN on listening socket → accept new connection
   - POLLIN on client socket → read request data
   - POLLOUT on client socket → write response data
   - POLLERR/POLLHUP → close connection
4. Cleanup inactive clients on timeout

## Building

```bash
make
```

This compiles the server with C++98 standard and strict warnings.

## Running

```bash
./webserv [port]
```

Default port is 8080 if not specified.

Example:
```bash
./webserv 8080
```

## Testing

Run the test suite:
```bash
./test_server.sh
```

Or test manually:
```bash
curl http://localhost:8080/
```

Test with multiple concurrent connections:
```bash
for i in {1..10}; do curl http://localhost:8080/ & done
```

## Implementation Details

### Per-Client Buffers

Each client maintains:
- `read_buffer`: Accumulates incoming request data
- `write_buffer`: Holds outgoing response data
- `write_pos`: Tracks partial write progress
- `last_activity`: For timeout detection

### Request Completion Detection

- Looks for "\r\n\r\n" (end of HTTP headers)
- If Content-Length present, waits for full body
- Otherwise assumes no body (GET/HEAD)

### Avoiding Infinite Hang

Multiple safeguards:
1. **poll() timeout**: 1 second timeout prevents indefinite blocking
2. **Client timeouts**: 30 second inactivity timeout
3. **Request size limit**: 8KB maximum request size
4. **Non-blocking I/O**: All sockets use O_NONBLOCK
5. **Error detection**: POLLERR/POLLHUP handled properly

### Non-Blocking I/O

All sockets are set to non-blocking mode using:
```cpp
fcntl(fd, F_SETFL, O_NONBLOCK)
```

This ensures recv()/send() return immediately with EAGAIN/EWOULDBLOCK if no data is available.

### POLLIN / POLLOUT Strategy

Events are set based on client state:
- **READING_REQUEST**: Monitor POLLIN (data available to read)
- **WRITING_RESPONSE**: Monitor POLLOUT (space available to write)
- **PROCESSING**: No events (CPU-bound, immediate)

This minimizes unnecessary wake-ups and improves efficiency.

## Files

- `Client.hpp`: Client state and buffer structures
- `Server.hpp`: Server class declaration
- `Server.cpp`: Event loop and request handling implementation
- `main.cpp`: Entry point
- `POLL_DESIGN.md`: Detailed design documentation
- `test_server.sh`: Test suite
- `Makefile`: Build configuration

## Limitations

This is a simple demonstration server. For production use, consider:
- Proper HTTP parsing (currently simplified)
- Keep-alive connection support
- Multiple listening sockets
- Configuration file support
- More sophisticated routing
- Static file serving
- CGI support
- Error logging
- Signal handling

## C++98 Compliance

This code uses only C++98 features:
- STL containers: `std::vector`, `std::map`, `std::string`
- No auto, nullptr, range-based for, lambdas, etc.
- Traditional iterators and function pointers
- POSIX system calls: poll(), fcntl(), socket(), etc.

## License

This is example code for educational purposes.