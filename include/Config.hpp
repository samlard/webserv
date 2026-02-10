#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include "ServerConfig.hpp"

class Config {
public:
    Config();
    Config(const Config& other);
    Config& operator=(const Config& other);
    ~Config();

    // Getters
    const std::vector<ServerConfig>& getServers() const;

    // Setters
    void addServer(const ServerConfig& server);

private:
    std::vector<ServerConfig> _servers;
};

#endif
