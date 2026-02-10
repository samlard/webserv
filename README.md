# WebServ - C++98 HTTP Server

A fully functional HTTP/1.1 server implementation in C++98 for the 42 School project.

## Features

- ✅ **C++98 compliant** - Compiles with `-std=c++98 -Wall -Wextra -Werror`
- ✅ **Non-blocking I/O** - Single `poll()` loop for all socket operations
- ✅ **Multiple ports** - Support for multiple listening ports
- ✅ **HTTP methods** - GET, POST, DELETE support
- ✅ **Static files** - Efficient static file serving with proper MIME types
- ✅ **Directory listing** - Auto-index feature
- ✅ **File uploads** - POST file upload support
- ✅ **CGI support** - Execute CGI scripts (fork + execve)
- ✅ **Configuration** - nginx-inspired configuration file
- ✅ **Keep-alive** - HTTP persistent connections
- ✅ **Error handling** - Proper HTTP status codes
- ✅ **No memory leaks** - Clean resource management
- ✅ **No crashes** - Robust error handling

## Architecture

### Core Classes

1. **Server** - Main orchestrator running the poll() loop
2. **Client** - Manages individual client connection state
3. **Request** - HTTP request parser (incremental, non-blocking)
4. **Response** - HTTP response builder
5. **Config** - Configuration file parser
6. **CGI** - CGI script execution handler
7. **Utils** - Helper functions for file/string operations

### Data Flow

```
Socket → Server (poll) → Client (read) → Request (parse) 
  → Process → Response (build) → Client (write) → Socket
```

### Non-Blocking State Machine

```
Client States:
READING_REQUEST → PROCESSING → WRITING_RESPONSE → DONE
       ↓                                            ↓
     ERROR                                    Keep-Alive?
                                                    ↓
                                              Reset & Reuse
```

## Building

```bash
make
```

This creates the `webserv` executable.

## Running

```bash
./webserv config/test.conf
```

The server will start and listen on the configured ports.

## Configuration

Configuration syntax is inspired by nginx:

```nginx
server {
    listen 8080;
    server_name localhost;
    root ./www;
    index index.html;
    client_max_body_size 10485760;
    
    error_page 404 /404.html;
    
    location / {
        allow_methods GET POST DELETE;
        autoindex on;
    }
    
    location /uploads {
        upload_path ./uploads;
        allow_methods GET POST DELETE;
    }
    
    location /cgi-bin {
        cgi_extension .py;
        cgi_path /usr/bin/python;
    }
}
```

### Configuration Directives

#### Server Block
- `listen` - Port number to listen on
- `server_name` - Server name (for Host header matching)
- `root` - Root directory for files
- `index` - Default index file
- `client_max_body_size` - Maximum request body size in bytes
- `error_page` - Custom error page for status code

#### Location Block
- `allow_methods` - Allowed HTTP methods (GET, POST, DELETE)
- `autoindex` - Enable directory listing (on/off)
- `root` - Override root directory for this location
- `index` - Override index file for this location
- `return` - Redirect to another location
- `upload_path` - Directory for file uploads
- `cgi_extension` - File extension for CGI scripts
- `cgi_path` - Path to CGI interpreter

## Testing

### Test GET request
```bash
curl http://localhost:8080/
```

### Test POST file upload
```bash
curl -X POST --data-binary "@file.txt" http://localhost:8080/uploads
```

### Test DELETE
```bash
curl -X DELETE http://localhost:8080/test.txt
```

### Test directory listing
```bash
curl http://localhost:8080/
```

## Project Structure

```
webserv/
├── includes/          # Header files
│   ├── Server.hpp
│   ├── Client.hpp
│   ├── Request.hpp
│   ├── Response.hpp
│   ├── Config.hpp
│   ├── CGI.hpp
│   └── Utils.hpp
├── srcs/             # Source files
│   ├── main.cpp
│   ├── Server.cpp
│   ├── Client.cpp
│   ├── Request.cpp
│   ├── Response.cpp
│   ├── Config.cpp
│   ├── CGI.cpp
│   └── Utils.cpp
├── config/           # Configuration files
│   ├── default.conf
│   └── test.conf
├── www/              # Default web root
│   └── index.html
├── Makefile
└── README.md
```

## Design Principles

### Single Responsibility
Each class has one clear purpose:
- Server: I/O multiplexing and orchestration
- Client: Connection state management
- Request: HTTP parsing
- Response: HTTP formatting
- Config: Configuration management
- CGI: Script execution
- Utils: Helper utilities

### Non-Blocking Architecture
- All socket operations are non-blocking
- Single poll() manages all file descriptors
- Incremental request parsing
- Partial write handling
- Timeout management

### No errno Dependencies
The design avoids relying on errno after read/write operations. Return values indicate success/failure directly.

### Memory Safety
- No raw pointers without ownership
- Proper cleanup in destructors
- RAII principles where applicable in C++98
- No memory leaks (valgrind clean)

### Robustness
- Graceful error handling
- Proper HTTP status codes
- Request validation
- Timeout handling
- Signal handling (SIGINT, SIGTERM)

## CGI Support

CGI scripts are executed using fork() and execve():
- Environment variables set per CGI/1.1 spec
- Request body piped to CGI stdin
- CGI stdout captured and parsed
- Timeout protection
- Proper cleanup

Example Python CGI script:
```python
#!/usr/bin/env python
print("Content-Type: text/html")
print()
print("<h1>Hello from CGI!</h1>")
```

## Limitations (by design)

- C++98 only (no C++11/14/17 features)
- No external libraries
- fork() only for CGI (not for handling requests)
- Single-threaded (poll-based event loop)
- No chunked transfer encoding (yet)
- No HTTPS support

## Development

Compile with debug info:
```bash
make CXXFLAGS="-std=c++98 -Wall -Wextra -Werror -g -Iincludes"
```

Check for memory leaks:
```bash
valgrind --leak-check=full ./webserv config/test.conf
```

## License

This is an educational project for 42 School.