*This project has been created as part of the 42 curriculum by [ssoumil](https://github.com/samlard), [mvan-vel](https://github.com/maximevanvelthoven), and [nicleena](https://github.com/SoLeQz).*

# webserv

## Description

**webserv** is a fully functional HTTP/1.1 web server written in C++98, built from scratch as part of the 42 school curriculum. The goal of the project is to gain a deep understanding of how web servers work under the hood — from socket management and I/O multiplexing, to HTTP request parsing and response generation.

Inspired by the behavior of NGINX, this server is driven by a configuration file and is capable of handling multiple simultaneous clients without ever blocking. It supports static file serving, file uploads, directory listing, custom error pages, and CGI execution for dynamic content generation.

### Key Features

- **HTTP/1.1 compliant** — supports `GET`, `POST`, and `DELETE` methods
- **Non-blocking I/O** — uses `poll()` (or equivalent) to handle multiple connections concurrently
- **Configuration file** — NGINX-inspired syntax for defining servers, ports, routes, and behaviors
- **Multiple virtual hosts** — serve different content based on host/port combinations
- **Static file serving** — serve HTML, CSS, images, and other assets from a root directory
- **Directory listing** — auto-generated index pages for directories when no index file is found
- **File uploads** — clients can upload files to the server via `POST`
- **CGI support** — execute scripts (e.g., Python, PHP) and return their output dynamically
- **Custom error pages** — configurable pages for 4xx and 5xx HTTP errors
- **Redirections** — HTTP redirects configurable per route

---

## Instructions

### Requirements

- A Unix-like operating system (Linux or macOS)
- `g++` or `clang++` with C++98 support
- `make`
- Python 3 (optional, for CGI script testing)
- PHP-CGI (optional, for PHP CGI testing)

### Compilation

Clone the repository and compile using the provided Makefile:

```bash
git clone https://github.com/<your-org>/webserv.git
cd webserv
make
```

This produces a `webserv` executable in the root directory.

To remove compiled objects:
```bash
make clean
```

To remove all build artifacts including the binary:
```bash
make fclean
```

To recompile from scratch:
```bash
make re
```

### Running the Server

```bash
./webserv [configuration_file]
```

If no configuration file is provided, the server will fall back to a default configuration.

**Example:**
```bash
./webserv config/default.conf
```

### Configuration File

The configuration file follows an NGINX-inspired syntax. Below is a minimal example:

```nginx
server {
    listen       8080;
    server_name  localhost;

    root         ./www;
    index        index.html;

    client_max_body_size 10M;

    error_page   404 /errors/404.html;
    error_page   500 /errors/500.html;

    location / {
        allow_methods GET POST;
        autoindex     off;
    }

    location /upload {
        allow_methods POST;
        upload_dir    ./www/uploads;
    }

    location /cgi-bin {
        allow_methods GET POST;
        cgi_extension .py;
        cgi_path      /usr/bin/python3;
    }
}
```

### Testing

You can test the server using a web browser, `curl`, or tools like `siege` for load testing:

```bash
# Basic GET request
curl -v http://localhost:8080/

# POST file upload
curl -v -X POST --data-binary @file.txt http://localhost:8080/upload

# DELETE request
curl -v -X DELETE http://localhost:8080/upload/file.txt
```

---

## Resources

### HTTP & Networking

- [RFC 9110 — HTTP Semantics](https://www.rfc-editor.org/rfc/rfc9110) — the authoritative reference for HTTP/1.1 semantics
- [RFC 9112 — HTTP/1.1](https://www.rfc-editor.org/rfc/rfc9112) — covers the wire format and message framing
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) — an excellent practical guide to BSD sockets in C
- [MDN Web Docs — HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP) — approachable HTTP reference for headers, methods, and status codes
- [NGINX Documentation](https://nginx.org/en/docs/) — used as behavioral reference for configuration syntax and server logic

### I/O Multiplexing

- [`poll(2)` man page](https://man7.org/linux/man-pages/man2/poll.2.html)
- [`select(2)` man page](https://man7.org/linux/man-pages/man2/select.2.html)
- [`epoll(7)` man page](https://man7.org/linux/man-pages/man7/epoll.7.html) — Linux-specific, higher-performance alternative

### CGI

- [RFC 3875 — The Common Gateway Interface (CGI/1.1)](https://www.rfc-editor.org/rfc/rfc3875) — specification for CGI script execution