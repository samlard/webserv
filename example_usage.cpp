#include "HttpRequest.hpp"
#include <iostream>
#include <string>
#include <sstream>

// Example demonstrating how the parser would be used in a real web server
// This simulates reading data from a socket in chunks

void simulateSocketRead(HttpRequest& req, const std::string& fullRequest, size_t chunkSize)
{
    std::cout << "Simulating socket read with chunk size: " << chunkSize << " bytes\n" << std::endl;
    
    size_t offset = 0;
    int iteration = 1;
    
    while (offset < fullRequest.length()) {
        size_t remaining = fullRequest.length() - offset;
        size_t toRead = (remaining < chunkSize) ? remaining : chunkSize;
        
        std::string chunk = fullRequest.substr(offset, toRead);
        
        std::cout << "Iteration " << iteration++ << ": Received " << toRead << " bytes" << std::endl;
        
        HttpRequest::ParseResult result = req.parse(chunk);
        
        if (result == HttpRequest::PARSE_COMPLETE) {
            std::cout << "✓ Request parsing complete!\n" << std::endl;
            break;
        } else if (result == HttpRequest::PARSE_ERROR) {
            std::cout << "✗ Parse error: " << req.getErrorMessage() << "\n" << std::endl;
            break;
        } else {
            std::cout << "  → Need more data...\n" << std::endl;
        }
        
        offset += toRead;
    }
}

int main()
{
    std::cout << "======================================" << std::endl;
    std::cout << "HTTP Request Parser - Usage Examples" << std::endl;
    std::cout << "======================================\n" << std::endl;
    
    // Example 1: Simple GET request with small chunks
    std::cout << "Example 1: GET request (10 byte chunks)" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    {
        HttpRequest req;
        std::string request = 
            "GET /api/users?id=123 HTTP/1.1\r\n"
            "Host: api.example.com\r\n"
            "User-Agent: Mozilla/5.0\r\n"
            "Accept: application/json\r\n"
            "\r\n";
        
        simulateSocketRead(req, request, 10);
        
        std::cout << "Parsed Request:" << std::endl;
        std::cout << "  Method: " << req.getMethod() << std::endl;
        std::cout << "  URI: " << req.getUri() << std::endl;
        std::cout << "  Host: " << req.getHeader("host") << std::endl;
        std::cout << "  User-Agent: " << req.getHeader("user-agent") << std::endl;
        std::cout << std::endl;
    }
    
    // Example 2: POST request with JSON body
    std::cout << "Example 2: POST request with JSON body" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    {
        HttpRequest req;
        std::string jsonBody = "{\"username\":\"john\",\"email\":\"john@example.com\"}";
        std::ostringstream oss;
        oss << jsonBody.length();
        
        std::string request = 
            "POST /api/users HTTP/1.1\r\n"
            "Host: api.example.com\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + oss.str() + "\r\n"
            "\r\n" +
            jsonBody;
        
        simulateSocketRead(req, request, 32);
        
        std::cout << "Parsed Request:" << std::endl;
        std::cout << "  Method: " << req.getMethod() << std::endl;
        std::cout << "  URI: " << req.getUri() << std::endl;
        std::cout << "  Content-Type: " << req.getHeader("content-type") << std::endl;
        std::cout << "  Body: " << req.getBody() << std::endl;
        std::cout << std::endl;
    }
    
    // Example 3: Chunked POST request
    std::cout << "Example 3: Chunked transfer encoding" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    {
        HttpRequest req;
        std::string request = 
            "POST /api/upload HTTP/1.1\r\n"
            "Host: api.example.com\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "18\r\n"
            "This is the first chunk.\r\n"
            "19\r\n"
            "This is the second chunk.\r\n"
            "0\r\n"
            "\r\n";
        
        simulateSocketRead(req, request, 20);
        
        std::cout << "Parsed Request:" << std::endl;
        std::cout << "  Method: " << req.getMethod() << std::endl;
        std::cout << "  URI: " << req.getUri() << std::endl;
        std::cout << "  Transfer-Encoding: " << req.getHeader("transfer-encoding") << std::endl;
        std::cout << "  Body (unchunked): " << req.getBody() << std::endl;
        std::cout << "  Body length: " << req.getBody().length() << " bytes" << std::endl;
        std::cout << std::endl;
    }
    
    // Example 4: DELETE request
    std::cout << "Example 4: DELETE request" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    {
        HttpRequest req;
        std::string request = 
            "DELETE /api/users/456 HTTP/1.1\r\n"
            "Host: api.example.com\r\n"
            "Authorization: Bearer token123\r\n"
            "\r\n";
        
        simulateSocketRead(req, request, 40);
        
        std::cout << "Parsed Request:" << std::endl;
        std::cout << "  Method: " << req.getMethod() << std::endl;
        std::cout << "  URI: " << req.getUri() << std::endl;
        std::cout << "  Authorization: " << req.getHeader("authorization") << std::endl;
        std::cout << std::endl;
    }
    
    // Example 5: Malformed request (error handling)
    std::cout << "Example 5: Error handling (malformed request)" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;
    {
        HttpRequest req;
        std::string request = "INVALID REQUEST\r\n\r\n";
        
        HttpRequest::ParseResult result = req.parse(request);
        
        if (result == HttpRequest::PARSE_ERROR) {
            std::cout << "✓ Error detected correctly!" << std::endl;
            std::cout << "  Status Code: " << req.getStatusCode() << std::endl;
            std::cout << "  Error Message: " << req.getErrorMessage() << std::endl;
        }
        std::cout << std::endl;
    }
    
    // Example 6: Body size limit enforcement
    std::cout << "Example 6: Body size limit enforcement" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    {
        HttpRequest req;
        req.setMaxBodySize(100); // Set very small limit
        
        std::string request = 
            "POST /api/upload HTTP/1.1\r\n"
            "Host: api.example.com\r\n"
            "Content-Length: 1000\r\n"
            "\r\n";
        
        HttpRequest::ParseResult result = req.parse(request);
        
        if (result == HttpRequest::PARSE_ERROR) {
            std::cout << "✓ Size limit enforced correctly!" << std::endl;
            std::cout << "  Status Code: " << req.getStatusCode() << std::endl;
            std::cout << "  Error Message: " << req.getErrorMessage() << std::endl;
        }
        std::cout << std::endl;
    }
    
    std::cout << "======================================" << std::endl;
    std::cout << "All examples completed successfully!" << std::endl;
    std::cout << "======================================" << std::endl;
    
    return 0;
}
