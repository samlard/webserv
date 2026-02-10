# HTTP/1.1 Request Parser - Implementation Summary

## Requirements Coverage

This implementation fully addresses all requirements specified in the problem statement:

### ✅ Parse request line (METHOD URI VERSION)
- **Implementation**: `parseRequestLine()` method in finite state machine
- **Location**: HttpRequest.cpp, lines 64-86
- **Details**: Uses `std::istringstream` to safely parse space-separated tokens
- **Validation**: Checks for empty fields, validates HTTP methods and versions

### ✅ Parse headers
- **Implementation**: `parseHeaders()` method with line-by-line parsing
- **Location**: HttpRequest.cpp, lines 88-140
- **Details**: 
  - Headers stored in `std::map<std::string, std::string>`
  - Header names normalized to lowercase for case-insensitive lookup
  - Validates header format (requires colon separator)
  - Detects end of headers with empty line
  - Extracts Content-Length and Transfer-Encoding automatically

### ✅ Support Content-Length
- **Implementation**: Body parsing with fixed length
- **Location**: HttpRequest.cpp, lines 142-152
- **Details**:
  - Parses Content-Length header value
  - Validates it's a valid number
  - Checks against max body size limit
  - Accumulates body data until Content-Length bytes received

### ✅ Support chunked transfer encoding (must unchunk before CGI)
- **Implementation**: Three-state chunked parser
- **Location**: HttpRequest.cpp, lines 154-234
- **States**:
  1. `CHUNK_SIZE`: Parse hexadecimal chunk size
  2. `CHUNK_DATA`: Read chunk data and verify CRLF
  3. `CHUNK_TRAILER`: Handle optional trailing headers
- **Details**:
  - Automatically unchunks data into body
  - Supports chunk extensions (parsed but ignored)
  - Validates CRLF after each chunk
  - Handles zero-length final chunk
  - Body is fully unchunked and ready for CGI

### ✅ Support GET, POST, DELETE
- **Implementation**: Method validation in `isValidMethod()`
- **Location**: HttpRequest.cpp, lines 300-303
- **Details**: 
  - Validates against allowed methods
  - Returns 400 Bad Request for invalid methods
  - POST requires Content-Length or Transfer-Encoding: chunked (returns 411 if missing)

### ✅ Must handle partial reads (data arriving in multiple recv calls)
- **Implementation**: Internal buffer accumulation
- **Location**: Throughout parse() method
- **Key Features**:
  - `_buffer` accumulates data across multiple `parse()` calls
  - State machine remembers position between calls
  - Handles incomplete lines, headers, and body chunks
  - No data loss between calls
  - Example: Can receive "GET /" in one call, " HTTP/1.1\r\n" in next

### ✅ Must detect malformed requests
- **Implementation**: Comprehensive validation throughout
- **Examples**:
  - Malformed request line (empty fields)
  - Invalid HTTP method
  - Invalid HTTP version
  - Malformed headers (missing colon, empty name)
  - Invalid Content-Length value
  - Invalid chunk size (non-hex, overflow)
  - Missing CRLF after chunks
  - Empty chunk size line

### ✅ Must return proper status codes
- **Implementation**: `setError()` method with status codes
- **Status Codes Returned**:
  - **400 Bad Request**: Malformed request line, invalid method/version, malformed headers, invalid Content-Length, invalid chunk size, missing CRLF
  - **411 Length Required**: POST request without Content-Length or Transfer-Encoding
  - **413 Request Entity Too Large**: Body size exceeds max limit
- **Location**: Error handling throughout HttpRequest.cpp

## Design Decisions

### Finite State Machine Approach
The parser uses a clear state machine with these states:
1. `REQUEST_LINE`: Parse METHOD URI VERSION
2. `HEADERS`: Parse headers until blank line
3. `BODY`: Read fixed-length body (Content-Length)
4. `CHUNK_SIZE`: Read chunk size (chunked)
5. `CHUNK_DATA`: Read chunk data (chunked)
6. `CHUNK_TRAILER`: Read trailing headers (chunked)
7. `COMPLETE`: Parsing complete
8. `ERROR`: Parse error occurred

**Benefits**:
- Clear separation of concerns
- Easy to debug and test
- Handles partial reads naturally
- Can resume at any state

### End of Headers Detection
Headers end when an empty line (`\r\n\r\n`) is encountered. The parser:
1. Reads lines until empty line found
2. Parses each header as "Name: Value"
3. On empty line, checks Content-Length and Transfer-Encoding
4. Transitions to appropriate body state (BODY, CHUNK_SIZE, or COMPLETE)

### Body Accumulation Safety
**Content-Length Bodies**:
- Checks size against limit before reading
- Accumulates in `std::string _body`
- Only completes when exact length received

**Chunked Bodies**:
- Accumulates chunks into `_body` as they arrive
- Checks cumulative size against limit
- Automatically unchunks (CGI receives plain body)
- Handles partial chunks (tracks `_currentChunkRead`)

### Max Body Size Enforcement
- Configurable via `setMaxBodySize()` (default 1MB)
- For Content-Length: Checked when headers complete
- For chunked: Checked before each chunk added
- Returns 413 Request Entity Too Large on violation

### Error Handling
- Single error path via `setError(code, message)`
- Parser stops immediately on first error
- Returns detailed error message for debugging
- Status code appropriate for HTTP responses

## Security Features

### Integer Overflow Protection
The `hexToSize()` function includes multiple protections:
1. **Length limit**: Max 16 hex digits (64-bit size_t)
2. **Overflow check**: Validates result won't overflow before multiplication
3. **Safe return**: Returns -1 (max size_t) on error

### Input Validation
- All input validated at parse time
- No buffer overflows (uses std::string)
- No unchecked array access
- Safe character handling with casts to unsigned char

### DoS Protection
- Max body size limit prevents memory exhaustion
- Early rejection of oversized requests
- Efficient buffer management

## Testing

### Test Coverage
14 comprehensive tests covering:
- All HTTP methods (GET, POST, DELETE)
- Both HTTP/1.0 and HTTP/1.1
- Content-Length and chunked encoding
- Partial reads at various boundaries
- All error conditions
- Edge cases (chunk extensions, long hex, etc.)

### Example Programs
- `test_parser.cpp`: Automated test suite
- `example_usage.cpp`: Real-world usage examples with simulated socket reads

## C++98 Compliance

Strictly follows C++98 standard:
- No auto keyword
- No range-based for loops
- No nullptr (uses NULL or 0)
- No lambda functions
- No std::unique_ptr or std::shared_ptr
- Standard containers only (std::string, std::map, std::vector)
- Compatible with g++ 3.x and later

## Performance Considerations

### Memory Efficiency
- Single buffer reused for all parsing
- Processed data removed from buffer
- Minimal copying with substr and append
- Headers stored efficiently in map

### CPU Efficiency
- Single pass parsing (no backtracking)
- State machine avoids redundant checks
- Early validation prevents wasted work
- Efficient string operations

## Integration with Web Server

This parser is designed for easy integration:

```cpp
// Pseudo-code for web server integration
while (true) {
    char buffer[8192];
    ssize_t n = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (n <= 0) break;
    
    HttpRequest::ParseResult result = request.parse(std::string(buffer, n));
    
    if (result == HttpRequest::PARSE_COMPLETE) {
        // Process request
        std::string method = request.getMethod();
        std::string uri = request.getUri();
        std::string body = request.getBody(); // Already unchunked!
        
        // Execute CGI or handle request
        handleRequest(request);
        break;
        
    } else if (result == HttpRequest::PARSE_ERROR) {
        // Send error response
        sendErrorResponse(request.getStatusCode(), request.getErrorMessage());
        break;
        
    } // else continue reading
}
```

## Files

- **HttpRequest.hpp**: Class definition, interface, and documentation
- **HttpRequest.cpp**: Complete implementation (370 lines)
- **test_parser.cpp**: Comprehensive test suite (330 lines)
- **example_usage.cpp**: Real-world usage examples (200 lines)
- **Makefile**: Build configuration
- **README.md**: User documentation
