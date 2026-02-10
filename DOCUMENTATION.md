# Nginx-Inspired Configuration Parser

A robust configuration parser for web server configurations, implemented in C++98 with clean parsing logic and comprehensive error handling.

## Features

- **Multiple server blocks**: Support for configuring multiple servers in one file
- **Multiple ports**: Each server can listen on multiple ports
- **Route configuration**: Flexible location/route blocks with:
  - Allowed HTTP methods
  - Root directory
  - Autoindex on/off
  - Index file specification
  - Upload path configuration
  - URL redirection
  - CGI extension mapping
- **Max body size**: Configure maximum request body size per server
- **Default error pages**: Custom error pages for different HTTP status codes

## Design

### Grammar Structure

The configuration file follows a hierarchical structure:

```
config       ::= server*
server       ::= "server" "{" server_directive* "}"
server_directive ::= listen | server_name | client_max_body_size | error_page | location

location     ::= "location" path "{" location_directive* "}"
location_directive ::= allow_methods | root | autoindex | index | upload_path | return | cgi_ext

listen       ::= "listen" port ";"
server_name  ::= "server_name" string ";"
client_max_body_size ::= "client_max_body_size" number ";"
error_page   ::= "error_page" number path ";"

allow_methods ::= "allow_methods" method+ ";"
root         ::= "root" path ";"
autoindex    ::= "autoindex" boolean ";"
index        ::= "index" filename ";"
upload_path  ::= "upload_path" path ";"
return       ::= "return" path ";"
cgi_ext      ::= "cgi_ext" extension cgi_path ";"

path         ::= string
method       ::= "GET" | "POST" | "PUT" | "DELETE" | ...
boolean      ::= "on" | "off" | "true" | "false" | "yes" | "no"
```

### Tokenizer Strategy

The tokenizer converts the input text into a stream of tokens:

1. **Whitespace handling**: Skips spaces, tabs, newlines
2. **Comment support**: Lines starting with `#` are ignored
3. **Token types**:
   - `TOKEN_WORD`: Identifiers and keywords (e.g., `server`, `location`)
   - `TOKEN_NUMBER`: Integer values
   - `TOKEN_STRING`: Quoted strings (single or double quotes)
   - `TOKEN_SEMICOLON`: Statement terminator `;`
   - `TOKEN_LBRACE`: Block opener `{`
   - `TOKEN_RBRACE`: Block closer `}`
   - `TOKEN_EOF`: End of file
   - `TOKEN_ERROR`: Invalid tokens

### Parser Structure

The parser uses a recursive descent approach:

1. **ConfigParser**: Main parser class
   - `parseConfig()`: Parses entire configuration
   - `parseServer()`: Parses a server block
   - `parseRoute()`: Parses a location block
   - `parseServerDirective()`: Parses server-level directives
   - `parseRouteDirective()`: Parses location-level directives

2. **Error handling**:
   - Throws `ParseException` with line/column information
   - Provides meaningful error messages
   - Never crashes on malformed input

### Data Structures

Three main classes store the parsed configuration:

1. **Config**: Top-level container
   - Contains multiple `ServerConfig` objects

2. **ServerConfig**: Server block configuration
   - Ports (vector of integers)
   - Server name (string)
   - Max body size (size_t)
   - Error pages (map: error code -> path)
   - Routes (vector of `RouteConfig`)

3. **RouteConfig**: Location block configuration
   - Path (string)
   - Allowed methods (vector of strings)
   - Root directory (string)
   - Autoindex flag (boolean)
   - Index file (string)
   - Upload path (string)
   - Redirection (string)
   - CGI extensions (map: extension -> CGI path)

### Validation Rules

The parser enforces several validation rules:

1. **Syntax validation**:
   - Proper block nesting
   - Correct semicolon placement
   - Valid directive names
   - Proper token types for values

2. **Semantic validation**:
   - Boolean values must be `on/off`, `true/false`, or `yes/no`
   - Numbers must be valid integers
   - Strings can be quoted or unquoted (if no spaces)

3. **Error handling**:
   - All errors include line and column numbers
   - Clear error messages describe the problem
   - Parser never crashes, always throws exceptions

## Usage

### Building

```bash
make
```

### Running

```bash
./webserv_parser <config_file>
```

### Example

```bash
./webserv_parser examples/config.conf
```

### Testing

```bash
make test
```

## Configuration Examples

### Minimal Configuration

```nginx
server {
    listen 8080;
    server_name localhost;

    location / {
        allow_methods GET;
        root /var/www;
        index index.html;
    }
}
```

### Full Configuration

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

    location /uploads {
        allow_methods GET POST DELETE;
        root /var/www/uploads;
        upload_path /var/www/uploads;
        autoindex off;
    }

    location /cgi-bin {
        allow_methods GET POST;
        root /var/www/cgi-bin;
        cgi_ext .py /usr/bin/python3;
        cgi_ext .php /usr/bin/php-cgi;
    }

    location /redirect {
        return /new-location;
    }
}
```

## Project Structure

```
webserv/
├── include/
│   ├── Config.hpp
│   ├── ServerConfig.hpp
│   ├── RouteConfig.hpp
│   ├── Token.hpp
│   ├── Tokenizer.hpp
│   └── ConfigParser.hpp
├── src/
│   ├── Config.cpp
│   ├── ServerConfig.cpp
│   ├── RouteConfig.cpp
│   ├── Token.cpp
│   ├── Tokenizer.cpp
│   ├── ConfigParser.cpp
│   └── main.cpp
├── examples/
│   ├── config.conf
│   └── minimal.conf
├── Makefile
└── DOCUMENTATION.md
```

## Supported Directives

### Server-Level Directives

- `listen <port>`: Specify a port to listen on (can be used multiple times)
- `server_name <name>`: Set the server name
- `client_max_body_size <bytes>`: Set maximum request body size in bytes
- `error_page <code> <path>`: Map error code to custom error page

### Location-Level Directives

- `allow_methods <method> [<method> ...]`: List of allowed HTTP methods
- `root <path>`: Set root directory for this location
- `autoindex <on|off>`: Enable/disable directory listing
- `index <filename>`: Set default index file
- `upload_path <path>`: Set upload directory path
- `return <path>`: Set redirection target
- `cgi_ext <extension> <cgi_path>`: Map file extension to CGI interpreter

## Error Handling

The parser provides comprehensive error handling:

1. **File I/O errors**: Reports if config file cannot be opened
2. **Tokenization errors**: Detects invalid characters and unterminated strings
3. **Syntax errors**: Reports missing braces, semicolons, or invalid structure
4. **Semantic errors**: Detects unknown directives and invalid values
5. **Error messages**: Include line and column numbers for easy debugging

Example error message:
```
Parse error at line 5, column 12: Expected ';' after listen directive
```

## Constraints Met

✅ **No regex**: Parser uses simple character-by-character tokenization  
✅ **Syntax error detection**: Comprehensive error checking with meaningful messages  
✅ **No crashes**: All errors are caught and reported via exceptions  
✅ **Clean parsing logic**: Recursive descent parser with clear structure  
✅ **Avoids spaghetti parsing**: Well-organized into tokenizer and parser phases  
✅ **C++98 compliant**: Uses only C++98 standard features  

## Future Enhancements

Possible improvements for future versions:

- Support for include directives
- Environment variable expansion
- More advanced validation (e.g., port range checking)
- Configuration reloading without restart
- SSL/TLS configuration directives
- Proxy and reverse proxy settings
