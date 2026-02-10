#include "../include/ConfigParser.hpp"
#include "../include/Tokenizer.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>

ParseException::ParseException(const std::string& message) :
    std::runtime_error(message) {}

ConfigParser::ConfigParser() : _position(0) {}

ConfigParser::~ConfigParser() {}

Config ConfigParser::parse(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        throw ParseException("Failed to open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return parseString(buffer.str());
}

Config ConfigParser::parseString(const std::string& content) {
    Tokenizer tokenizer(content);
    _tokens = tokenizer.tokenize();
    _position = 0;
    
    return parseConfig();
}

Token ConfigParser::peek() const {
    if (_position < _tokens.size())
        return _tokens[_position];
    return _tokens[_tokens.size() - 1]; // Return EOF token
}

Token ConfigParser::advance() {
    if (_position < _tokens.size()) {
        return _tokens[_position++];
    }
    return _tokens[_tokens.size() - 1]; // Return EOF token
}

void ConfigParser::expect(TokenType type, const std::string& message) {
    Token token = peek();
    if (token.getType() != type) {
        std::stringstream ss;
        ss << "Parse error at line " << token.getLine()
           << ", column " << token.getColumn() << ": " << message;
        throw ParseException(ss.str());
    }
    advance();
}

bool ConfigParser::match(TokenType type) {
    if (peek().getType() == type) {
        advance();
        return true;
    }
    return false;
}

bool ConfigParser::isAtEnd() const {
    return peek().getType() == TOKEN_EOF;
}

std::string ConfigParser::parseStringValue() {
    Token token = peek();
    if (token.getType() == TOKEN_STRING || token.getType() == TOKEN_WORD) {
        advance();
        return token.getValue();
    }
    
    std::stringstream ss;
    ss << "Expected string value at line " << token.getLine();
    throw ParseException(ss.str());
}

int ConfigParser::parseIntValue() {
    Token token = peek();
    if (token.getType() == TOKEN_NUMBER) {
        advance();
        return std::atoi(token.getValue().c_str());
    }
    
    std::stringstream ss;
    ss << "Expected integer value at line " << token.getLine();
    throw ParseException(ss.str());
}

bool ConfigParser::parseBoolValue() {
    Token token = peek();
    if (token.getType() == TOKEN_WORD) {
        std::string value = token.getValue();
        advance();
        
        if (value == "on" || value == "true" || value == "yes")
            return true;
        if (value == "off" || value == "false" || value == "no")
            return false;
    }
    
    std::stringstream ss;
    ss << "Expected boolean value (on/off) at line " << token.getLine();
    throw ParseException(ss.str());
}

std::vector<std::string> ConfigParser::parseList() {
    std::vector<std::string> list;
    
    while (peek().getType() == TOKEN_WORD || peek().getType() == TOKEN_STRING) {
        list.push_back(parseStringValue());
    }
    
    return list;
}

Config ConfigParser::parseConfig() {
    Config config;
    
    while (!isAtEnd()) {
        Token token = peek();
        
        if (token.getType() == TOKEN_ERROR) {
            std::stringstream ss;
            ss << "Invalid token at line " << token.getLine()
               << ", column " << token.getColumn() << ": " << token.getValue();
            throw ParseException(ss.str());
        }
        
        if (token.getType() == TOKEN_WORD && token.getValue() == "server") {
            config.addServer(parseServer());
        } else {
            std::stringstream ss;
            ss << "Unexpected token at line " << token.getLine()
               << ": expected 'server', got '" << token.getValue() << "'";
            throw ParseException(ss.str());
        }
    }
    
    return config;
}

ServerConfig ConfigParser::parseServer() {
    ServerConfig server;
    
    expect(TOKEN_WORD, "Expected 'server'");
    expect(TOKEN_LBRACE, "Expected '{' after 'server'");
    
    while (peek().getType() != TOKEN_RBRACE && !isAtEnd()) {
        Token token = peek();
        
        if (token.getType() == TOKEN_WORD && token.getValue() == "location") {
            server.addRoute(parseRoute());
        } else {
            parseServerDirective(server);
        }
    }
    
    expect(TOKEN_RBRACE, "Expected '}' to close server block");
    
    return server;
}

void ConfigParser::parseServerDirective(ServerConfig& server) {
    Token directive = peek();
    
    if (directive.getType() != TOKEN_WORD) {
        std::stringstream ss;
        ss << "Expected directive at line " << directive.getLine();
        throw ParseException(ss.str());
    }
    
    std::string name = directive.getValue();
    advance();
    
    if (name == "listen") {
        int port = parseIntValue();
        server.addPort(port);
        expect(TOKEN_SEMICOLON, "Expected ';' after listen directive");
    } else if (name == "server_name") {
        std::string serverName = parseStringValue();
        server.setServerName(serverName);
        expect(TOKEN_SEMICOLON, "Expected ';' after server_name directive");
    } else if (name == "client_max_body_size") {
        int size = parseIntValue();
        server.setMaxBodySize(static_cast<size_t>(size));
        expect(TOKEN_SEMICOLON, "Expected ';' after client_max_body_size directive");
    } else if (name == "error_page") {
        int errorCode = parseIntValue();
        std::string path = parseStringValue();
        server.addErrorPage(errorCode, path);
        expect(TOKEN_SEMICOLON, "Expected ';' after error_page directive");
    } else {
        std::stringstream ss;
        ss << "Unknown server directive '" << name << "' at line " << directive.getLine();
        throw ParseException(ss.str());
    }
}

RouteConfig ConfigParser::parseRoute() {
    RouteConfig route;
    
    expect(TOKEN_WORD, "Expected 'location'");
    
    std::string path = parseStringValue();
    route.setPath(path);
    
    expect(TOKEN_LBRACE, "Expected '{' after location path");
    
    while (peek().getType() != TOKEN_RBRACE && !isAtEnd()) {
        parseRouteDirective(route);
    }
    
    expect(TOKEN_RBRACE, "Expected '}' to close location block");
    
    return route;
}

void ConfigParser::parseRouteDirective(RouteConfig& route) {
    Token directive = peek();
    
    if (directive.getType() != TOKEN_WORD) {
        std::stringstream ss;
        ss << "Expected directive at line " << directive.getLine();
        throw ParseException(ss.str());
    }
    
    std::string name = directive.getValue();
    advance();
    
    if (name == "allow_methods") {
        std::vector<std::string> methods = parseList();
        for (size_t i = 0; i < methods.size(); i++) {
            route.addAllowedMethod(methods[i]);
        }
        expect(TOKEN_SEMICOLON, "Expected ';' after allow_methods directive");
    } else if (name == "root") {
        std::string root = parseStringValue();
        route.setRoot(root);
        expect(TOKEN_SEMICOLON, "Expected ';' after root directive");
    } else if (name == "autoindex") {
        bool autoindex = parseBoolValue();
        route.setAutoindex(autoindex);
        expect(TOKEN_SEMICOLON, "Expected ';' after autoindex directive");
    } else if (name == "index") {
        std::string index = parseStringValue();
        route.setIndex(index);
        expect(TOKEN_SEMICOLON, "Expected ';' after index directive");
    } else if (name == "upload_path") {
        std::string uploadPath = parseStringValue();
        route.setUploadPath(uploadPath);
        expect(TOKEN_SEMICOLON, "Expected ';' after upload_path directive");
    } else if (name == "return") {
        std::string redirection = parseStringValue();
        route.setRedirection(redirection);
        expect(TOKEN_SEMICOLON, "Expected ';' after return directive");
    } else if (name == "cgi_ext") {
        std::string extension = parseStringValue();
        std::string cgiPath = parseStringValue();
        route.addCgiExtension(extension, cgiPath);
        expect(TOKEN_SEMICOLON, "Expected ';' after cgi_ext directive");
    } else {
        std::stringstream ss;
        ss << "Unknown location directive '" << name << "' at line " << directive.getLine();
        throw ParseException(ss.str());
    }
}
