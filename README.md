# webserv

A C++98 HTTP web server with full CGI support implementing non-blocking I/O and poll-based event loop.

## Features

- **HTTP/1.1 Support**: Parses GET and POST requests with proper header handling
- **CGI Execution**: Full CGI/1.1 implementation with fork/execve
- **Chunked Transfer Encoding**: Handles chunked requests and unchunks before sending to CGI
- **Non-blocking I/O**: Uses poll() for efficient multiplexed I/O
- **Multiple Concurrent Connections**: Handles multiple clients and CGI processes simultaneously
- **Standards Compliant**: Passes correct CGI environment variables per CGI/1.1 spec

## Building

```bash
make        # Build the server
make clean  # Remove object files
make fclean # Remove all build artifacts
make re     # Rebuild from scratch
```

## Usage

```bash
./webserv [port]
```

Default port is 8080 if not specified.

## Testing CGI Scripts

The server looks for CGI scripts in the `cgi-bin/` directory. URLs starting with `/cgi-bin/` are treated as CGI requests.

### Included Test Scripts

- `test.cgi` - Simple C CGI binary
- `test.py` - Python CGI script
- `test.sh` - Bash CGI script

### Example Requests

```bash
# GET request
curl http://localhost:8080/cgi-bin/test.py

# GET with query string
curl "http://localhost:8080/cgi-bin/test.py?name=John&age=30"

# POST request
curl -X POST -d "username=alice&password=secret" http://localhost:8080/cgi-bin/test.py

# Chunked POST
curl -X POST -H "Transfer-Encoding: chunked" --data-binary @file.txt \
  http://localhost:8080/cgi-bin/test.py
```

## Architecture

See [CGI_IMPLEMENTATION.md](CGI_IMPLEMENTATION.md) for detailed documentation on:
- Pipe setup and management
- Non-blocking I/O handling
- CGI process lifecycle
- Environment variable construction
- Response parsing and merging

## Implementation Details

- **Language**: C++98 (strict compliance)
- **System Calls**: fork(), execve(), pipe(), poll(), fcntl()
- **Constraints**: 
  - fork() only used for CGI execution
  - Non-blocking pipes integrated with poll loop
  - Proper EOF handling and process reaping
  - Full chunked encoding support
