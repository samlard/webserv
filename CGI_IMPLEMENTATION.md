# CGI Implementation Documentation

## Overview
This webserver implements CGI/1.1 specification with non-blocking I/O integrated into a poll-based event loop.

## Pipe Setup

The CGI handler uses three pipes for communication with the CGI process:

```cpp
int _pipeIn[2];   // Parent writes to CGI stdin
int _pipeOut[2];  // Parent reads from CGI stdout  
int _pipeErr[2];  // Parent reads from CGI stderr
```

### Pipe Creation (setupPipes())
1. Create three pipes using `pipe()` system call
2. In child process (CGI):
   - Redirect `_pipeIn[0]` to `STDIN_FILENO` using `dup2()`
   - Redirect `_pipeOut[1]` to `STDOUT_FILENO` using `dup2()`
   - Redirect `_pipeErr[1]` to `STDERR_FILENO` using `dup2()`
   - Close all pipe file descriptors
3. In parent process:
   - Close child's ends: `_pipeIn[0]`, `_pipeOut[1]`, `_pipeErr[1]`
   - Keep parent's ends: `_pipeIn[1]` (write), `_pipeOut[0]` (read), `_pipeErr[0]` (read)

## Non-Blocking Pipe Handling

All parent-side pipe file descriptors are set to non-blocking mode using:

```cpp
void CGIHandler::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```

### Writing to CGI stdin (writeToStdin())
- Writes request body in chunks to prevent blocking
- Handles `EAGAIN`/`EWOULDBLOCK` errors (would block)
- Tracks bytes written with `_bodyWritten` counter
- Closes pipe when all data written or on error

### Reading from CGI stdout (readFromStdout())
- Reads in 4KB chunks to prevent blocking
- Handles `EAGAIN`/`EWOULDBLOCK` errors (no data available)
- Accumulates data in `_responseData` string
- Detects EOF (read returns 0) and closes pipe

## CGI Lifecycle Management

### 1. Execution (execute())
```
1. Setup pipes
2. Set parent's pipe ends to non-blocking
3. Get request body (unchunked if chunked encoding)
4. fork() child process
5. Child: redirect stdio, build environment, execve()
6. Parent: close child's pipe ends, mark as running
```

### 2. Integration with Poll Loop
The server adds CGI pipe file descriptors to the poll array:
- `getStdinFd()`: Monitor with `POLLOUT` when data to write
- `getStdoutFd()`: Monitor with `POLLIN` to read output

### 3. I/O Processing
On each poll event:
- `POLLOUT` on stdin fd → call `writeToStdin()`
- `POLLIN` on stdout fd → call `readFromStdout()`

### 4. Completion Detection (isDone())
CGI is done when:
- Child process has exited (`waitpid()` with `WNOHANG`)
- Stdin pipe is closed (all request body sent)
- Stdout pipe is closed (all output read)

### 5. Cleanup
- Remove CGI pipes from poll array
- Parse CGI output into HTTP response
- Delete CGI handler object
- Send response to client

## Building Environment Variables

The `buildEnvironment()` method creates CGI/1.1 compliant environment:

### Required Variables
- `GATEWAY_INTERFACE=CGI/1.1`
- `SERVER_PROTOCOL=HTTP/1.1` (from request)
- `SERVER_SOFTWARE=WebServ/1.0`
- `REQUEST_METHOD=GET|POST|...` (from request)

### Path Variables
- `SCRIPT_NAME`: URI path (before query string)
- `SCRIPT_FILENAME`: Absolute path to CGI script
- `QUERY_STRING`: URI query parameters (after ?)
- `PATH_INFO`: URI path
- `PATH_TRANSLATED`: Resolved script path

### Content Variables
- `CONTENT_TYPE`: From Content-Type header
- `CONTENT_LENGTH`: From Content-Length header or computed for chunked

### HTTP Headers
All HTTP headers converted to `HTTP_*` format:
- Convert to uppercase
- Replace hyphens with underscores
- Prepend with `HTTP_`
- Example: `Content-Type` → `HTTP_CONTENT_TYPE`

### Server Variables
- `SERVER_NAME=localhost`
- `SERVER_PORT=8080`
- `REMOTE_ADDR=127.0.0.1`

## Handling Chunked Requests

### Detection
```cpp
std::string transferEncoding = getHeader("Transfer-Encoding");
_isChunked = (Utils::toLower(transferEncoding) == "chunked");
```

### Unchunking Process (unchunkBody())
1. Read chunk size in hex
2. Read chunk size + 2 bytes (data + CRLF)
3. Append chunk data to result
4. Repeat until chunk size is 0
5. Send unchunked body to CGI via stdin

### Why Unchunk?
CGI scripts expect regular body data, not chunked encoding. The server handles the HTTP transfer encoding layer.

## Merging CGI Output into HTTP Response

### CGI Output Format
```
Header1: Value1
Header2: Value2
Status: 200 OK

Body content here
```

### Parsing (parseCGIOutput())
1. Find `\r\n\r\n` separator between headers and body
2. Parse header section line by line
3. Extract special headers:
   - `Status`: Sets HTTP status code and message
   - Other headers: Added to response headers
4. Extract body (everything after separator)
5. Build HTTP response with parsed data

### Handling Missing Content-Length
If CGI doesn't provide `Content-Length`:
- Server reads all output until EOF
- Calculates length from accumulated data
- Adds `Content-Length` header automatically in `HTTPResponse::build()`

## Non-Blocking Server Integration

### Why Non-Blocking?
- Server handles multiple connections simultaneously
- CGI execution can take time
- Server must remain responsive to other clients

### Poll Loop Integration
```
1. Client sends request
2. Server detects CGI request
3. Fork and execute CGI
4. Add CGI pipes to poll array
5. Handle I/O events as they occur:
   - Write request body incrementally
   - Read response incrementally
6. When CGI completes, send response to client
```

### Key Benefits
- No blocking on CGI I/O
- Supports concurrent CGI executions
- Server processes other requests while waiting for CGI

## Usage Example

### Start Server
```bash
make
./webserv 8080
```

### Test with curl
```bash
# GET request
curl http://localhost:8080/cgi-bin/test.py

# POST request
curl -X POST -d "test=data" http://localhost:8080/cgi-bin/test.py

# Chunked POST
curl -X POST -H "Transfer-Encoding: chunked" --data-binary @file.txt \
  http://localhost:8080/cgi-bin/test.py
```

### CGI Scripts
Place executable scripts in `cgi-bin/` directory:
- Must have execute permissions (`chmod +x`)
- Must output valid CGI headers
- Can be any language (Python, Bash, Perl, etc.)

## Security Considerations

1. **fork() Only for CGI**: Fork is only called in CGI execution, not for regular requests
2. **execve() Usage**: Uses `execve()` with full path to prevent PATH injection
3. **Environment Isolation**: Each CGI gets fresh environment variables
4. **Resource Limits**: Consider adding timeouts and resource limits in production
5. **Input Validation**: Validate URI paths to prevent directory traversal

## Error Handling

- Pipe creation failures: Throws exception, cleans up partial pipes
- Fork failures: Returns false, server sends 500 error
- execve failures: Child exits with status 1
- I/O errors: Closes affected pipes, continues with available data
- CGI crashes: Detected via waitpid(), sends accumulated output

## Testing

The repository includes test CGI scripts:
- `cgi-bin/test.py`: Python script echoing environment and POST data
- `cgi-bin/test.sh`: Bash script generating HTML response

Both scripts demonstrate:
- Reading environment variables
- Reading POST data from stdin
- Outputting CGI headers
- Generating response body
