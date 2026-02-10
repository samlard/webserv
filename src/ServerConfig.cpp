#include "../include/ServerConfig.hpp"

ServerConfig::ServerConfig() : _maxBodySize(1048576) {} // Default 1MB

ServerConfig::ServerConfig(const ServerConfig& other) :
    _ports(other._ports),
    _serverName(other._serverName),
    _routes(other._routes),
    _maxBodySize(other._maxBodySize),
    _errorPages(other._errorPages) {}

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {
        _ports = other._ports;
        _serverName = other._serverName;
        _routes = other._routes;
        _maxBodySize = other._maxBodySize;
        _errorPages = other._errorPages;
    }
    return *this;
}

ServerConfig::~ServerConfig() {}

const std::vector<int>& ServerConfig::getPorts() const { return _ports; }
const std::string& ServerConfig::getServerName() const { return _serverName; }
const std::vector<RouteConfig>& ServerConfig::getRoutes() const { return _routes; }
size_t ServerConfig::getMaxBodySize() const { return _maxBodySize; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const { return _errorPages; }

void ServerConfig::addPort(int port) { _ports.push_back(port); }
void ServerConfig::setServerName(const std::string& serverName) { _serverName = serverName; }
void ServerConfig::addRoute(const RouteConfig& route) { _routes.push_back(route); }
void ServerConfig::setMaxBodySize(size_t maxBodySize) { _maxBodySize = maxBodySize; }
void ServerConfig::addErrorPage(int errorCode, const std::string& path) {
    _errorPages[errorCode] = path;
}
