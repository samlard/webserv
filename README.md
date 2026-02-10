# webserv

A nginx-inspired configuration parser implemented in C++98 for web server configuration files.

## Features

✅ **Multiple server blocks** - Configure multiple servers in one file  
✅ **Multiple ports** - Each server can listen on multiple ports  
✅ **Route configuration** - Flexible location/route blocks with:
- Allowed HTTP methods (GET, POST, PUT, DELETE, etc.)
- Root directory
- Autoindex on/off for directory listing
- Index file specification
- Upload path configuration
- URL redirection
- CGI extension mapping

✅ **Max body size** - Configure maximum request body size per server  
✅ **Default error pages** - Custom error pages for different HTTP status codes  
✅ **Robust error handling** - Comprehensive syntax and semantic validation  
✅ **Clean parsing logic** - Well-structured tokenizer and recursive descent parser  
✅ **C++98 compliant** - No C++11 features, pure C++98 standard

## Quick Start

### Build

```bash
make
```

### Run Parser

```bash
./webserv_parser examples/config.conf
```

### Run Tests

```bash
make test
```

### Run Demo

```bash
make demo
```

## Configuration Example

```nginx
server {
    listen 8080;
    listen 8443;
    server_name example.com;
    client_max_body_size 10485760;
    error_page 404 /errors/404.html;
    error_page 500 /errors/500.html;

    location / {
        allow_methods GET POST;
        root /var/www/html;
        autoindex on;
        index index.html;
    }

    location /cgi-bin {
        allow_methods GET POST;
        root /var/www/cgi-bin;
        cgi_ext .py /usr/bin/python3;
        cgi_ext .php /usr/bin/php-cgi;
    }
}
```

## Supported Directives

### Server-Level
- `listen <port>` - Port to listen on
- `server_name <name>` - Server name
- `client_max_body_size <bytes>` - Max request body size
- `error_page <code> <path>` - Custom error page

### Location-Level
- `allow_methods <method>...` - Allowed HTTP methods
- `root <path>` - Root directory
- `autoindex <on|off>` - Directory listing
- `index <file>` - Default index file
- `upload_path <path>` - Upload directory
- `return <path>` - Redirection target
- `cgi_ext <ext> <path>` - CGI interpreter mapping

## Documentation

See [DOCUMENTATION.md](DOCUMENTATION.md) for complete grammar specification, design details, and usage instructions.

## Project Structure

```
webserv/
├── include/          # Header files
├── src/              # Source files
├── tests/            # Test suite
├── examples/         # Example configurations
├── Makefile          # Build system
└── DOCUMENTATION.md  # Complete documentation
```

## Requirements Met

✅ Multiple server blocks  
✅ Multiple ports per server  
✅ Route configuration with all requested features  
✅ Max body size configuration  
✅ Default error pages  
✅ No regex used  
✅ Comprehensive syntax error detection  
✅ No crashes on malformed config  
✅ Clean, maintainable parsing logic  
✅ C++98 compliant

## License

This project is part of the 42 school curriculum.