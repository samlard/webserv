#include "../include/ConfigParser.hpp"
#include <iostream>
#include <exception>

void printConfig(const Config& config) {
    const std::vector<ServerConfig>& servers = config.getServers();
    
    std::cout << "Configuration loaded successfully!" << std::endl;
    std::cout << "Number of servers: " << servers.size() << std::endl << std::endl;
    
    for (size_t i = 0; i < servers.size(); i++) {
        const ServerConfig& server = servers[i];
        std::cout << "Server " << (i + 1) << ":" << std::endl;
        
        // Print ports
        std::cout << "  Ports: ";
        const std::vector<int>& ports = server.getPorts();
        for (size_t j = 0; j < ports.size(); j++) {
            std::cout << ports[j];
            if (j < ports.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        // Print server name
        if (!server.getServerName().empty()) {
            std::cout << "  Server name: " << server.getServerName() << std::endl;
        }
        
        // Print max body size
        std::cout << "  Max body size: " << server.getMaxBodySize() << " bytes" << std::endl;
        
        // Print error pages
        const std::map<int, std::string>& errorPages = server.getErrorPages();
        if (!errorPages.empty()) {
            std::cout << "  Error pages:" << std::endl;
            for (std::map<int, std::string>::const_iterator it = errorPages.begin();
                 it != errorPages.end(); ++it) {
                std::cout << "    " << it->first << " -> " << it->second << std::endl;
            }
        }
        
        // Print routes
        const std::vector<RouteConfig>& routes = server.getRoutes();
        std::cout << "  Routes: " << routes.size() << std::endl;
        for (size_t j = 0; j < routes.size(); j++) {
            const RouteConfig& route = routes[j];
            std::cout << "    Location: " << route.getPath() << std::endl;
            
            const std::vector<std::string>& methods = route.getAllowedMethods();
            if (!methods.empty()) {
                std::cout << "      Allowed methods: ";
                for (size_t k = 0; k < methods.size(); k++) {
                    std::cout << methods[k];
                    if (k < methods.size() - 1) std::cout << ", ";
                }
                std::cout << std::endl;
            }
            
            if (!route.getRoot().empty()) {
                std::cout << "      Root: " << route.getRoot() << std::endl;
            }
            
            std::cout << "      Autoindex: " << (route.getAutoindex() ? "on" : "off") << std::endl;
            
            if (!route.getIndex().empty()) {
                std::cout << "      Index: " << route.getIndex() << std::endl;
            }
            
            if (!route.getUploadPath().empty()) {
                std::cout << "      Upload path: " << route.getUploadPath() << std::endl;
            }
            
            if (!route.getRedirection().empty()) {
                std::cout << "      Redirection: " << route.getRedirection() << std::endl;
            }
            
            const std::map<std::string, std::string>& cgiExts = route.getCgiExtensions();
            if (!cgiExts.empty()) {
                std::cout << "      CGI extensions:" << std::endl;
                for (std::map<std::string, std::string>::const_iterator it = cgiExts.begin();
                     it != cgiExts.end(); ++it) {
                    std::cout << "        " << it->first << " -> " << it->second << std::endl;
                }
            }
        }
        
        std::cout << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    
    try {
        ConfigParser parser;
        Config config = parser.parse(argv[1]);
        printConfig(config);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
