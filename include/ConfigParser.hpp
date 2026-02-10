#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include "Config.hpp"
#include "Token.hpp"

class ParseException : public std::runtime_error {
public:
    ParseException(const std::string& message);
};

class ConfigParser {
public:
    ConfigParser();
    ~ConfigParser();

    Config parse(const std::string& filename);
    Config parseString(const std::string& content);

private:
    std::vector<Token> _tokens;
    size_t _position;

    Config parseConfig();
    ServerConfig parseServer();
    RouteConfig parseRoute();
    void parseServerDirective(ServerConfig& server);
    void parseRouteDirective(RouteConfig& route);
    
    Token peek() const;
    Token advance();
    void expect(TokenType type, const std::string& message);
    bool match(TokenType type);
    bool isAtEnd() const;
    
    std::string parseStringValue();
    int parseIntValue();
    bool parseBoolValue();
    std::vector<std::string> parseList();
};

#endif
