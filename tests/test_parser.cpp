#include "../include/ConfigParser.hpp"
#include "../include/Tokenizer.hpp"
#include <iostream>
#include <cassert>
#include <exception>

// Test helpers
int tests_run = 0;
int tests_passed = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        tests_run++; \
        std::cout << "Running test: " #name << "... "; \
        try { \
            test_##name(); \
            tests_passed++; \
            std::cout << "PASSED" << std::endl; \
        } catch (const std::exception& e) { \
            std::cout << "FAILED: " << e.what() << std::endl; \
        } catch (...) { \
            std::cout << "FAILED: Unknown exception" << std::endl; \
        } \
    } \
    void test_##name()

// Test basic tokenization
TEST(tokenizer_basic) {
    Tokenizer tokenizer("server { listen 8080; }");
    std::vector<Token> tokens = tokenizer.tokenize();
    
    assert(tokens.size() == 7); // server, {, listen, 8080, ;, }, EOF
    assert(tokens[0].getType() == TOKEN_WORD);
    assert(tokens[0].getValue() == "server");
    assert(tokens[1].getType() == TOKEN_LBRACE);
    assert(tokens[2].getType() == TOKEN_WORD);
    assert(tokens[3].getType() == TOKEN_NUMBER);
    assert(tokens[4].getType() == TOKEN_SEMICOLON);
    assert(tokens[5].getType() == TOKEN_RBRACE);
    assert(tokens[6].getType() == TOKEN_EOF);
}

// Test comment handling
TEST(tokenizer_comments) {
    Tokenizer tokenizer("# comment\nserver { }");
    std::vector<Token> tokens = tokenizer.tokenize();
    
    assert(tokens[0].getValue() == "server");
}

// Test string tokenization
TEST(tokenizer_strings) {
    Tokenizer tokenizer("root \"test path\";");
    std::vector<Token> tokens = tokenizer.tokenize();
    
    assert(tokens[1].getType() == TOKEN_STRING);
    assert(tokens[1].getValue() == "test path");
}

// Test minimal config parsing
TEST(parse_minimal_config) {
    std::string config = 
        "server {\n"
        "    listen 8080;\n"
        "    location / {\n"
        "        root /var/www;\n"
        "    }\n"
        "}";
    
    ConfigParser parser;
    Config result = parser.parseString(config);
    
    assert(result.getServers().size() == 1);
    assert(result.getServers()[0].getPorts().size() == 1);
    assert(result.getServers()[0].getPorts()[0] == 8080);
    assert(result.getServers()[0].getRoutes().size() == 1);
}

// Test multiple servers
TEST(parse_multiple_servers) {
    std::string config = 
        "server { listen 8080; }\n"
        "server { listen 9090; }";
    
    ConfigParser parser;
    Config result = parser.parseString(config);
    
    assert(result.getServers().size() == 2);
}

// Test multiple ports
TEST(parse_multiple_ports) {
    std::string config = 
        "server {\n"
        "    listen 8080;\n"
        "    listen 8443;\n"
        "}";
    
    ConfigParser parser;
    Config result = parser.parseString(config);
    
    assert(result.getServers()[0].getPorts().size() == 2);
    assert(result.getServers()[0].getPorts()[0] == 8080);
    assert(result.getServers()[0].getPorts()[1] == 8443);
}

// Test allowed methods
TEST(parse_allowed_methods) {
    std::string config = 
        "server {\n"
        "    listen 8080;\n"
        "    location / {\n"
        "        allow_methods GET POST DELETE;\n"
        "    }\n"
        "}";
    
    ConfigParser parser;
    Config result = parser.parseString(config);
    
    const std::vector<std::string>& methods = 
        result.getServers()[0].getRoutes()[0].getAllowedMethods();
    assert(methods.size() == 3);
    assert(methods[0] == "GET");
    assert(methods[1] == "POST");
    assert(methods[2] == "DELETE");
}

// Test autoindex
TEST(parse_autoindex) {
    std::string config = 
        "server {\n"
        "    listen 8080;\n"
        "    location / {\n"
        "        autoindex on;\n"
        "    }\n"
        "}";
    
    ConfigParser parser;
    Config result = parser.parseString(config);
    
    assert(result.getServers()[0].getRoutes()[0].getAutoindex() == true);
}

// Test CGI extensions
TEST(parse_cgi_extensions) {
    std::string config = 
        "server {\n"
        "    listen 8080;\n"
        "    location /cgi {\n"
        "        cgi_ext .py /usr/bin/python;\n"
        "        cgi_ext .php /usr/bin/php-cgi;\n"
        "    }\n"
        "}";
    
    ConfigParser parser;
    Config result = parser.parseString(config);
    
    const std::map<std::string, std::string>& cgi = 
        result.getServers()[0].getRoutes()[0].getCgiExtensions();
    assert(cgi.size() == 2);
    assert(cgi.find(".py")->second == "/usr/bin/python");
    assert(cgi.find(".php")->second == "/usr/bin/php-cgi");
}

// Test error pages
TEST(parse_error_pages) {
    std::string config = 
        "server {\n"
        "    listen 8080;\n"
        "    error_page 404 /errors/404.html;\n"
        "    error_page 500 /errors/500.html;\n"
        "}";
    
    ConfigParser parser;
    Config result = parser.parseString(config);
    
    const std::map<int, std::string>& errors = 
        result.getServers()[0].getErrorPages();
    assert(errors.size() == 2);
    assert(errors.find(404)->second == "/errors/404.html");
    assert(errors.find(500)->second == "/errors/500.html");
}

// Test missing semicolon error
TEST(error_missing_semicolon) {
    std::string config = "server { listen 8080 }";
    
    ConfigParser parser;
    bool caught = false;
    try {
        parser.parseString(config);
    } catch (const ParseException&) {
        caught = true;
    }
    
    assert(caught);
}

// Test missing brace error
TEST(error_missing_brace) {
    std::string config = "server { listen 8080;";
    
    ConfigParser parser;
    bool caught = false;
    try {
        parser.parseString(config);
    } catch (const ParseException&) {
        caught = true;
    }
    
    assert(caught);
}

// Test unknown directive error
TEST(error_unknown_directive) {
    std::string config = "server { unknown_directive value; }";
    
    ConfigParser parser;
    bool caught = false;
    try {
        parser.parseString(config);
    } catch (const ParseException&) {
        caught = true;
    }
    
    assert(caught);
}

int main() {
    std::cout << "Running configuration parser tests...\n" << std::endl;
    
    run_test_tokenizer_basic();
    run_test_tokenizer_comments();
    run_test_tokenizer_strings();
    run_test_parse_minimal_config();
    run_test_parse_multiple_servers();
    run_test_parse_multiple_ports();
    run_test_parse_allowed_methods();
    run_test_parse_autoindex();
    run_test_parse_cgi_extensions();
    run_test_parse_error_pages();
    run_test_error_missing_semicolon();
    run_test_error_missing_brace();
    run_test_error_unknown_directive();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << "/" << tests_run << std::endl;
    std::cout << "========================================" << std::endl;
    
    return tests_passed == tests_run ? 0 : 1;
}
