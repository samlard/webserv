#include "HttpRequest.hpp"
#include <iostream>
#include <cassert>

void printRequest(const HttpRequest& req)
{
    std::cout << "=== HTTP Request ===" << std::endl;
    std::cout << "Method: " << req.getMethod() << std::endl;
    std::cout << "URI: " << req.getUri() << std::endl;
    std::cout << "Version: " << req.getVersion() << std::endl;
    std::cout << "Headers:" << std::endl;
    
    std::map<std::string, std::string> headers = req.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }
    
    std::cout << "Body (" << req.getBody().length() << " bytes): " << req.getBody() << std::endl;
    
    if (req.hasError()) {
        std::cout << "Error " << req.getStatusCode() << ": " << req.getErrorMessage() << std::endl;
    }
    std::cout << "===================" << std::endl << std::endl;
}

void testSimpleGet()
{
    std::cout << "Test 1: Simple GET request" << std::endl;
    HttpRequest req;
    
    std::string data = "GET /index.html HTTP/1.1\r\n"
                      "Host: example.com\r\n"
                      "User-Agent: TestClient/1.0\r\n"
                      "\r\n";
    
    HttpRequest::ParseResult result = req.parse(data);
    
    assert(result == HttpRequest::PARSE_COMPLETE);
    assert(req.isComplete());
    assert(!req.hasError());
    assert(req.getMethod() == "GET");
    assert(req.getUri() == "/index.html");
    assert(req.getVersion() == "HTTP/1.1");
    assert(req.getHeader("host") == "example.com");
    assert(req.getHeader("user-agent") == "TestClient/1.0");
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testPostWithContentLength()
{
    std::cout << "Test 2: POST request with Content-Length" << std::endl;
    HttpRequest req;
    
    std::string data = "POST /api/data HTTP/1.1\r\n"
                      "Host: example.com\r\n"
                      "Content-Length: 13\r\n"
                      "\r\n"
                      "Hello, World!";
    
    HttpRequest::ParseResult result = req.parse(data);
    
    assert(result == HttpRequest::PARSE_COMPLETE);
    assert(req.isComplete());
    assert(!req.hasError());
    assert(req.getMethod() == "POST");
    assert(req.getUri() == "/api/data");
    assert(req.getBody() == "Hello, World!");
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testPartialReads()
{
    std::cout << "Test 3: Partial reads (data arriving in chunks)" << std::endl;
    HttpRequest req;
    
    // Send data in multiple chunks
    assert(req.parse("GET /test") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse(" HTTP/1.1\r\n") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse("Host: example.com\r\n") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse("Content-Length: 5\r\n") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse("\r\n") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse("He") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse("llo") == HttpRequest::PARSE_COMPLETE);
    
    assert(req.isComplete());
    assert(req.getBody() == "Hello");
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testChunkedEncoding()
{
    std::cout << "Test 4: Chunked transfer encoding" << std::endl;
    HttpRequest req;
    
    std::string data = "POST /upload HTTP/1.1\r\n"
                      "Host: example.com\r\n"
                      "Transfer-Encoding: chunked\r\n"
                      "\r\n"
                      "5\r\n"
                      "Hello\r\n"
                      "7\r\n"
                      ", World\r\n"
                      "0\r\n"
                      "\r\n";
    
    HttpRequest::ParseResult result = req.parse(data);
    
    assert(result == HttpRequest::PARSE_COMPLETE);
    assert(req.isComplete());
    assert(req.getBody() == "Hello, World");
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testChunkedWithExtensions()
{
    std::cout << "Test 5: Chunked encoding with extensions" << std::endl;
    HttpRequest req;
    
    std::string data = "POST /upload HTTP/1.1\r\n"
                      "Host: example.com\r\n"
                      "Transfer-Encoding: chunked\r\n"
                      "\r\n"
                      "4;name=value\r\n"
                      "Test\r\n"
                      "0\r\n"
                      "\r\n";
    
    HttpRequest::ParseResult result = req.parse(data);
    
    assert(result == HttpRequest::PARSE_COMPLETE);
    assert(req.getBody() == "Test");
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testMalformedRequestLine()
{
    std::cout << "Test 6: Malformed request line" << std::endl;
    HttpRequest req;
    
    std::string data = "INVALID\r\n\r\n";
    
    HttpRequest::ParseResult result = req.parse(data);
    
    assert(result == HttpRequest::PARSE_ERROR);
    assert(req.hasError());
    assert(req.getStatusCode() == 400);
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testInvalidMethod()
{
    std::cout << "Test 7: Invalid method" << std::endl;
    HttpRequest req;
    
    std::string data = "INVALID /test HTTP/1.1\r\n\r\n";
    
    HttpRequest::ParseResult result = req.parse(data);
    
    assert(result == HttpRequest::PARSE_ERROR);
    assert(req.hasError());
    assert(req.getStatusCode() == 400);
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testPostWithoutContentLength()
{
    std::cout << "Test 8: POST without Content-Length (should return 411)" << std::endl;
    HttpRequest req;
    
    std::string data = "POST /test HTTP/1.1\r\n"
                      "Host: example.com\r\n"
                      "\r\n";
    
    HttpRequest::ParseResult result = req.parse(data);
    
    assert(result == HttpRequest::PARSE_ERROR);
    assert(req.hasError());
    assert(req.getStatusCode() == 411);
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testBodyTooLarge()
{
    std::cout << "Test 9: Body too large (should return 413)" << std::endl;
    HttpRequest req;
    req.setMaxBodySize(10); // Set small limit
    
    std::string data = "POST /test HTTP/1.1\r\n"
                      "Host: example.com\r\n"
                      "Content-Length: 100\r\n"
                      "\r\n";
    
    HttpRequest::ParseResult result = req.parse(data);
    
    assert(result == HttpRequest::PARSE_ERROR);
    assert(req.hasError());
    assert(req.getStatusCode() == 413);
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testDeleteMethod()
{
    std::cout << "Test 10: DELETE method" << std::endl;
    HttpRequest req;
    
    std::string data = "DELETE /resource/123 HTTP/1.1\r\n"
                      "Host: example.com\r\n"
                      "\r\n";
    
    HttpRequest::ParseResult result = req.parse(data);
    
    assert(result == HttpRequest::PARSE_COMPLETE);
    assert(req.isComplete());
    assert(req.getMethod() == "DELETE");
    assert(req.getUri() == "/resource/123");
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testMalformedHeader()
{
    std::cout << "Test 11: Malformed header (no colon)" << std::endl;
    HttpRequest req;
    
    std::string data = "GET /test HTTP/1.1\r\n"
                      "InvalidHeader\r\n"
                      "\r\n";
    
    HttpRequest::ParseResult result = req.parse(data);
    
    assert(result == HttpRequest::PARSE_ERROR);
    assert(req.hasError());
    assert(req.getStatusCode() == 400);
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testChunkedPartialReads()
{
    std::cout << "Test 12: Chunked encoding with partial reads" << std::endl;
    HttpRequest req;
    
    // Send chunked data in multiple pieces
    assert(req.parse("POST /test HTTP/1.1\r\n") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse("Transfer-Encoding: chunked\r\n") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse("\r\n") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse("5\r\n") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse("Hel") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse("lo\r\n") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse("0\r\n") == HttpRequest::PARSE_INCOMPLETE);
    assert(req.parse("\r\n") == HttpRequest::PARSE_COMPLETE);
    
    assert(req.isComplete());
    assert(req.getBody() == "Hello");
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testInvalidChunkSize()
{
    std::cout << "Test 13: Invalid chunk size" << std::endl;
    HttpRequest req;
    
    std::string data = "POST /test HTTP/1.1\r\n"
                      "Transfer-Encoding: chunked\r\n"
                      "\r\n"
                      "GGGG\r\n";
    
    HttpRequest::ParseResult result = req.parse(data);
    
    assert(result == HttpRequest::PARSE_ERROR);
    assert(req.hasError());
    assert(req.getStatusCode() == 400);
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

void testHttp10()
{
    std::cout << "Test 14: HTTP/1.0 request" << std::endl;
    HttpRequest req;
    
    std::string data = "GET /test HTTP/1.0\r\n"
                      "Host: example.com\r\n"
                      "\r\n";
    
    HttpRequest::ParseResult result = req.parse(data);
    
    assert(result == HttpRequest::PARSE_COMPLETE);
    assert(req.isComplete());
    assert(req.getVersion() == "HTTP/1.0");
    
    printRequest(req);
    std::cout << "PASSED\n" << std::endl;
}

int main()
{
    std::cout << "====================================" << std::endl;
    std::cout << "HTTP/1.1 Request Parser Test Suite" << std::endl;
    std::cout << "====================================" << std::endl << std::endl;
    
    try {
        testSimpleGet();
        testPostWithContentLength();
        testPartialReads();
        testChunkedEncoding();
        testChunkedWithExtensions();
        testMalformedRequestLine();
        testInvalidMethod();
        testPostWithoutContentLength();
        testBodyTooLarge();
        testDeleteMethod();
        testMalformedHeader();
        testChunkedPartialReads();
        testInvalidChunkSize();
        testHttp10();
        
        std::cout << "====================================" << std::endl;
        std::cout << "All tests passed!" << std::endl;
        std::cout << "====================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
