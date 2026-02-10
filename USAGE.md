# Usage Examples

This document provides practical examples of using and testing the poll-based HTTP server.

## Basic Usage

### Starting the Server

Default port (8080):
```bash
./webserv
```

Custom port:
```bash
./webserv 3000
```

The server will output:
```
Server listening on port 8080
Starting event loop...
```

## Testing Examples

### 1. Simple GET Request

```bash
curl http://localhost:8080/
```

Expected output:
```html
<html><body><h1>Hello from Poll-Based Server!</h1>
<p>Your request was processed successfully.</p>
<p>This server uses a single poll() call for all sockets.</p>
</body></html>
```

### 2. Viewing HTTP Headers

```bash
curl -v http://localhost:8080/
```

You'll see:
```
< HTTP/1.1 200 OK
< Content-Type: text/html
< Content-Length: 172
< Connection: close
```

### 3. POST Request with Body

```bash
curl -X POST -d "test data" http://localhost:8080/
```

The server will process the request and return the same response.

### 4. Concurrent Connections Test

Test with 10 simultaneous connections:
```bash
for i in {1..10}; do 
    curl -s http://localhost:8080/ & 
done
wait
echo "All requests completed"
```

### 5. Stress Test

Test with 100 sequential requests:
```bash
for i in {1..100}; do 
    curl -s http://localhost:8080/ > /dev/null
    echo "Request $i completed"
done
```

### 6. Using telnet for Manual Testing

Connect manually to see the raw protocol:
```bash
telnet localhost 8080
```

Then type:
```
GET / HTTP/1.1
Host: localhost

```
(Press Enter twice after the Host line)

### 7. Testing with wget

```bash
wget -O - http://localhost:8080/
```

### 8. Load Testing with Apache Bench (if installed)

```bash
ab -n 1000 -c 10 http://localhost:8080/
```

This sends 1000 requests with 10 concurrent connections.

## Server Behavior Examples

### Partial Read Handling

The server handles partial reads automatically. When a request arrives in multiple TCP packets:

1. First POLLIN: Receives first part, accumulates in buffer
2. Second POLLIN: Receives more data, continues accumulating
3. When "\r\n\r\n" detected: Request complete, processes it

### Partial Write Handling

For large responses that can't be sent in one call:

1. First POLLOUT: Sends as much as possible, updates write_pos
2. Second POLLOUT: Sends remaining data from write_pos
3. When all sent: Closes connection

### Timeout Handling

If a client connects but doesn't send data:
```bash
telnet localhost 8080
# Wait 30+ seconds without typing
```

Server output will show:
```
New connection: fd=X
Client timeout: fd=X
Closing connection: fd=X
```

### Request Too Large

Try sending a very large request:
```bash
dd if=/dev/zero bs=1024 count=10 | curl -X POST --data-binary @- http://localhost:8080/
```

Server will respond with:
```
HTTP/1.1 413 Payload Too Large
Content-Length: 0
Connection: close
```

## Server Log Output

Example server log during operation:

```
Server listening on port 8080
Starting event loop...
New connection: fd=4
Read 78 bytes from fd=4
Processing request from fd=4
Request: GET / HTTP/1.1
Wrote 247 bytes to fd=4 (247/247)
Closing connection: fd=4
```

## Using the Test Suite

Run all tests:
```bash
./test_server.sh
```

Expected output:
```
=== Testing Poll-Based HTTP Server ===

Test 1: Simple GET request
✓ PASS: Simple GET request works

Test 2: Multiple concurrent connections (10 requests)
✓ PASS: All 10 concurrent requests succeeded

...

=== Test Summary ===
```

## Advanced Testing

### Testing Non-Blocking Behavior

The server should handle slow clients without blocking fast ones:

```bash
# Terminal 1: Start server
./webserv 8080

# Terminal 2: Slow client (keeps connection open)
telnet localhost 8080

# Terminal 3: Fast client (should work immediately)
curl http://localhost:8080/
```

The fast client gets response immediately, proving non-blocking operation.

### Testing Connection Limit

Open many connections simultaneously:
```bash
for i in {1..50}; do 
    (telnet localhost 8080 &)
done
```

The server handles all connections using a single poll() call.

## Monitoring

### Check Server Process

```bash
ps aux | grep webserv
```

### Check Open Connections

```bash
netstat -an | grep 8080
```

or

```bash
lsof -i :8080
```

## Stopping the Server

The server runs until killed:
```bash
# Find PID
ps aux | grep webserv

# Stop it
kill <PID>
```

Or use Ctrl+C if running in foreground.

## Performance Notes

The poll-based architecture provides:
- **Low latency**: Events processed immediately
- **No busy-waiting**: CPU efficient
- **Scalable**: Handles many connections with single thread
- **Predictable**: Linear growth in connections vs performance

Typical performance on modern hardware:
- Can handle hundreds of concurrent connections
- Low CPU usage when idle
- Fast response times (<1ms for simple requests)

## Troubleshooting

### Port Already in Use

```
Error: Failed to bind to port
```

Solution: Use a different port or stop the process using that port.

### Connection Refused

```
curl: (7) Failed to connect to localhost port 8080: Connection refused
```

Solution: Make sure server is running.

### Timeout Issues

If clients timeout frequently:
- Check CLIENT_TIMEOUT_SECONDS in Server.hpp
- Increase timeout for slow clients
- Check network latency

## Further Reading

See:
- [POLL_DESIGN.md](POLL_DESIGN.md) - Architecture details
- [README.md](README.md) - Overview and building
- [SECURITY.md](SECURITY.md) - Security analysis
