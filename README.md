# WebServ - C++98 HTTP Server

A robust, non-blocking HTTP/1.1 server implementation in C++98 for 42 School project.

## Features

- ✅ **Strict Non-blocking I/O**: Single `poll()` call manages all socket events
- ✅ **HTTP Methods**: Full support for GET, POST, and DELETE
- ✅ **CGI Execution**: Run scripts via `fork()` + `execve()`
- ✅ **File Uploads**: Handle POST requests with configurable size limits
- ✅ **Multi-port**: Listen on multiple ports simultaneously
- ✅ **Configuration**: Nginx-like configuration file format
- ✅ **Directory Listing**: Optional autoindex for directories
- ✅ **Clean Architecture**: Modular design with clear separation of concerns
- ✅ **No External Libraries**: Pure C++98, no Boost or external dependencies

## Architecture

### Core Components

```
Server (poll event loop)
  ├── Socket (non-blocking wrapper)
  ├── Client (connection state)
  ├── HttpRequest (request parser)
  ├── HttpResponse (response builder)
  ├── RequestHandler (GET/POST/DELETE logic)
  ├── CgiHandler (fork+execve CGI)
  └── Config (nginx-like parser)
```

### Key Design Decisions

1. **Single Poll Loop**: All I/O operations use one `poll()` call
2. **State Machine Parser**: HTTP request parsing is incremental and non-blocking
3. **No Threads**: Pure event-driven architecture
4. **RAII**: Socket and Client objects manage their own resources
5. **Modular Handlers**: Request processing separated by concern

## Building

```bash
make        # Build webserv
make clean  # Remove object files
make fclean # Remove all build artifacts
make re     # Rebuild from scratch
```

## Configuration

Configuration file format (nginx-like):

```nginx
server {
    listen 8080
    listen 8081
    server_name localhost
    client_max_body_size 10485760
    autoindex on
    
    location / root ./www
    location /uploads root ./www/uploads
    
    cgi_extension .py /usr/bin/python3
    cgi_extension .sh /bin/bash
}
```

### Configuration Directives

- `listen <port>` - Port to listen on (can have multiple)
- `server_name <name>` - Server name(s)
- `client_max_body_size <bytes>` - Max request body size
- `autoindex on|off` - Enable/disable directory listings
- `location <path> root <directory>` - Map URL paths to filesystem
- `cgi_extension <ext> <interpreter>` - CGI script handlers

## Running

```bash
# Use default config
./webserv

# Use custom config
./webserv path/to/config.conf
```

## Testing

### Manual Testing

```bash
# Start server
./webserv

# Test GET
curl http://localhost:8080/

# Test POST (upload)
curl -X POST -d "test data" http://localhost:8080/uploads/test.txt

# Test DELETE
curl -X DELETE http://localhost:8080/uploads/test.txt

# Test CGI
curl http://localhost:8080/test.py?param=value
```

### Browser Testing

Open `http://localhost:8080/` in a browser to see the interactive test page.

## Implementation Details

### Non-blocking I/O

All sockets are set to `O_NONBLOCK` mode. The server uses a single `poll()` call to monitor:
- Listening sockets for new connections (POLLIN)
- Client sockets for incoming data (POLLIN)
- Client sockets ready for writing responses (POLLOUT)

### Request Processing Flow

1. Accept new connection → Create Client object
2. Read data → Parse into HttpRequest
3. Request complete → Process with RequestHandler or CgiHandler
4. Generate HttpResponse
5. Write response data
6. Close connection when complete

### CGI Execution

CGI scripts are executed safely:
1. `fork()` creates child process
2. Setup pipes for stdout capture
3. Build environment variables (REQUEST_METHOD, QUERY_STRING, etc.)
4. `execve()` runs interpreter with script
5. Parent reads output via pipe
6. `waitpid()` ensures no zombies

### Error Handling

- All system calls checked for errors
- Graceful degradation on resource exhaustion
- Proper cleanup via RAII
- No crashes on malformed requests

## Project Structure

```
.
├── Makefile
├── config/
│   └── default.conf       # Default configuration
├── includes/
│   ├── Config.hpp         # Configuration parser
│   ├── Socket.hpp         # Non-blocking socket wrapper
│   ├── Server.hpp         # Main server class
│   ├── Client.hpp         # Client connection handler
│   ├── HttpRequest.hpp    # HTTP request parser
│   ├── HttpResponse.hpp   # HTTP response builder
│   ├── RequestHandler.hpp # Request processing
│   └── CgiHandler.hpp     # CGI execution
├── sources/
│   ├── main.cpp
│   ├── Config.cpp
│   ├── Socket.cpp
│   ├── Server.cpp
│   ├── Client.cpp
│   ├── HttpRequest.cpp
│   ├── HttpResponse.cpp
│   ├── RequestHandler.cpp
│   └── CgiHandler.cpp
└── www/
    ├── index.html         # Test page
    ├── test.txt          # Test file
    ├── test.py           # CGI test script
    └── uploads/          # Upload directory
```

## Compliance

- ✅ C++98 standard
- ✅ No external libraries (except standard library)
- ✅ No Boost
- ✅ Strict non-blocking I/O
- ✅ Single poll() for all sockets
- ✅ No threads
- ✅ Clean compilation with -Wall -Wextra -Werror

## Known Limitations

- Single server config used for all ports (simplified for demo)
- Basic content-type detection
- No HTTPS support
- No chunked transfer encoding
- No HTTP/2

## Future Enhancements

- Virtual host support (multiple server configs by hostname)
- Chunked transfer encoding
- More sophisticated MIME type detection
- Request timeouts
- Connection keepalive
- Partial content (Range requests)

## Author

42 School - WebServ Project

## License

This project is part of 42 School curriculum.