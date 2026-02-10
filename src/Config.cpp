#include "../include/Config.hpp"

Config::Config() {}

Config::Config(const Config& other) : _servers(other._servers) {}

Config& Config::operator=(const Config& other) {
    if (this != &other) {
        _servers = other._servers;
    }
    return *this;
}

Config::~Config() {}

const std::vector<ServerConfig>& Config::getServers() const { return _servers; }

void Config::addServer(const ServerConfig& server) { _servers.push_back(server); }
