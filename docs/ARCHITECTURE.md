# WebServ Architecture Documentation

## Design Overview

This HTTP server follows a single-threaded, event-driven architecture using `poll()` for non-blocking I/O multiplexing.

## Core Components

### 1. Server - Orchestrator
Main event loop and socket management. Sets up listening sockets, runs poll() loop, accepts connections, routes requests.

### 2. Client - Connection State
Manages individual client connections with state machine for non-blocking I/O. Handles partial reads/writes and timeouts.

### 3. Request - Parser
Parses HTTP requests incrementally (request line, headers, body) with state tracking for non-blocking operation.

### 4. Response - Builder
Builds HTTP responses with status codes, headers, and body content.

### 5. Config - Configuration Parser
Parses nginx-style configuration files with server blocks and location blocks.

### 6. CGI - Script Executor
Executes CGI scripts using fork/execve with proper environment and I/O piping.

### 7. Utils - Helper Functions
String manipulation, file operations, path utilities, MIME types, URL encoding/decoding.

## Data Flow

```
Socket → poll() → Server → Client (read) → Request (parse)
  → Process (GET/POST/DELETE) → Response (build)
  → Client (write) → Socket
```

## Non-Blocking Architecture

- All sockets set to O_NONBLOCK
- Single poll() manages all file descriptors
- Incremental request parsing
- Partial write handling
- State machines for connection lifecycle
- No blocking calls except fork/execve for CGI

## Key Design Decisions

1. **Single-threaded**: C++98 compliant, no threading needed
2. **poll() over select()**: Better scalability, no FD_SETSIZE limit
3. **State machines**: Enable non-blocking operation
4. **Separate classes**: Single Responsibility Principle
5. **RAII**: Automatic resource cleanup

See full architecture details in docs/ARCHITECTURE.md
