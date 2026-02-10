#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include "RouteConfig.hpp"

class ServerConfig {
public:
    ServerConfig();
    ServerConfig(const ServerConfig& other);
    ServerConfig& operator=(const ServerConfig& other);
    ~ServerConfig();

    // Getters
    const std::vector<int>& getPorts() const;
    const std::string& getServerName() const;
    const std::vector<RouteConfig>& getRoutes() const;
    size_t getMaxBodySize() const;
    const std::map<int, std::string>& getErrorPages() const;

    // Setters
    void addPort(int port);
    void setServerName(const std::string& serverName);
    void addRoute(const RouteConfig& route);
    void setMaxBodySize(size_t maxBodySize);
    void addErrorPage(int errorCode, const std::string& path);

private:
    std::vector<int> _ports;
    std::string _serverName;
    std::vector<RouteConfig> _routes;
    size_t _maxBodySize;
    std::map<int, std::string> _errorPages;
};

#endif
