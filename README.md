# webserv

HTTP/1.1 Request Parser Implementation in C++98

## Overview

This project implements a robust HTTP/1.1 request parser following the HTTP/1.1 specification (RFC 2616). The parser uses a finite state machine approach to handle partial reads and supports all common HTTP features needed for a web server.

## Features

- ✅ **Request Line Parsing**: Parses METHOD, URI, and HTTP VERSION
- ✅ **Header Parsing**: Extracts and stores all HTTP headers (case-insensitive)
- ✅ **Content-Length Support**: Handles fixed-length request bodies
- ✅ **Chunked Transfer Encoding**: Fully supports chunked encoding with automatic unchunking
- ✅ **Multiple HTTP Methods**: GET, POST, DELETE
- ✅ **Partial Reads**: Handles data arriving in multiple `recv()` calls
- ✅ **Malformed Request Detection**: Detects and reports various malformed requests
- ✅ **Proper Status Codes**: Returns appropriate HTTP status codes:
  - `400 Bad Request` - Malformed request line, invalid headers, invalid chunk size
  - `411 Length Required` - POST without Content-Length or Transfer-Encoding
  - `413 Request Entity Too Large` - Body exceeds max size limit
- ✅ **Configurable Limits**: Max body size can be configured (default 1MB)

## Design

### Finite State Machine

The parser uses a state machine with the following states:

1. **REQUEST_LINE**: Parsing the initial request line (METHOD URI VERSION)
2. **HEADERS**: Parsing HTTP headers until blank line
3. **BODY**: Reading fixed-length body (Content-Length)
4. **CHUNK_SIZE**: Reading chunk size line (chunked encoding)
5. **CHUNK_DATA**: Reading chunk data (chunked encoding)
6. **CHUNK_TRAILER**: Reading optional trailer headers after last chunk
7. **COMPLETE**: Request fully parsed
8. **ERROR**: Parse error occurred

### Buffer Management

- Accumulates data in an internal buffer across multiple `parse()` calls
- Efficiently handles partial reads without data loss
- Automatically removes processed data from buffer

### Error Handling

- Clean error detection with appropriate HTTP status codes
- Detailed error messages for debugging
- Safe failure mode - parser stops on first error

## Building

```bash
make
```

## Testing

Run the comprehensive test suite:

```bash
./test_parser
```

The test suite includes 14 tests covering:
- Simple GET requests
- POST with Content-Length
- Partial reads (data in chunks)
- Chunked transfer encoding
- Chunk extensions
- Various error conditions
- All three HTTP methods (GET, POST, DELETE)
- HTTP/1.0 and HTTP/1.1

## Usage Example

```cpp
#include "HttpRequest.hpp"

// Create parser instance
HttpRequest request;

// Configure max body size (optional)
request.setMaxBodySize(1048576); // 1MB

// Parse incoming data (can be called multiple times)
HttpRequest::ParseResult result = request.parse(data);

if (result == HttpRequest::PARSE_COMPLETE) {
    // Request is complete
    std::string method = request.getMethod();
    std::string uri = request.getUri();
    std::string body = request.getBody();
    std::string host = request.getHeader("host");
    
} else if (result == HttpRequest::PARSE_ERROR) {
    // Handle error
    int statusCode = request.getStatusCode();
    std::string error = request.getErrorMessage();
    
} else {
    // Need more data - call parse() again with next chunk
}
```

## API Reference

### Main Methods

- `ParseResult parse(const std::string& data)` - Parse incoming data
- `bool isComplete() const` - Check if parsing is complete
- `bool hasError() const` - Check if an error occurred

### Accessors

- `std::string getMethod() const` - Get HTTP method
- `std::string getUri() const` - Get request URI
- `std::string getVersion() const` - Get HTTP version
- `std::string getHeader(const std::string& name) const` - Get header value (case-insensitive)
- `std::string getBody() const` - Get request body (unchunked if chunked encoding)
- `int getStatusCode() const` - Get error status code (if error)
- `std::string getErrorMessage() const` - Get error message (if error)

### Configuration

- `void setMaxBodySize(size_t size)` - Set maximum body size in bytes

## Implementation Details

### Chunked Transfer Encoding

The parser automatically unchunks chunked request bodies:
- Parses chunk size (hexadecimal)
- Handles chunk extensions (ignores them)
- Validates CRLF after each chunk
- Accumulates chunks into body
- Handles trailer headers
- Enforces max body size across all chunks

### Partial Read Handling

The parser is designed for real-world socket programming:
- Buffers incomplete lines and data
- Continues parsing when more data arrives
- No data loss between calls
- Efficient memory management

### Security Features

- Max body size enforcement prevents DoS
- Input validation at every stage
- Safe string operations (no buffer overflows)
- Protection against malformed requests

## C++98 Compliance

This implementation strictly follows C++98 standard:
- No C++11 features used
- Compatible with older compilers
- Portable across platforms

## Files

- `HttpRequest.hpp` - Class definition and interface
- `HttpRequest.cpp` - Implementation
- `test_parser.cpp` - Comprehensive test suite
- `Makefile` - Build configuration