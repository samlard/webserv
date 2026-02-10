#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

/**
 * ServerConfig - Configuration for a single server block
 */
struct ServerConfig {
	std::vector<int> ports;
	std::string serverName;
	std::string root;
	std::string index;
	size_t clientMaxBodySize;
	std::map<int, std::string> errorPages;
	
	// Location configurations
	struct Location {
		std::string path;
		std::vector<std::string> allowedMethods;
		std::string root;
		std::string index;
		bool autoindex;
		std::string redirect;
		std::string uploadPath;
		
		// CGI
		bool cgiEnabled;
		std::string cgiExtension;
		std::string cgiPath;
		
		Location() : autoindex(false), cgiEnabled(false) {}
	};
	
	std::vector<Location> locations;
	
	ServerConfig() : clientMaxBodySize(1048576) {} // 1MB default
	
	// Find best matching location for a URI
	const Location* findLocation(const std::string& uri) const;
};

/**
 * Config - Parses and holds server configuration
 * 
 * Responsibilities:
 * - Parse nginx-style configuration file
 * - Validate configuration
 * - Store server blocks with their settings
 * - Provide access to configuration data
 */
class Config {
public:
	Config();
	~Config();

	// Parse configuration file
	bool parseFile(const std::string& filepath);
	
	// Get configurations
	const std::vector<ServerConfig>& getServers() const;
	
	// Error handling
	const std::string& getError() const;

private:
	std::vector<ServerConfig> _servers;
	std::string _error;
	
	// Parsing helpers
	bool parseServerBlock(const std::string& content, size_t& pos);
	bool parseLocationBlock(ServerConfig& server, ServerConfig::Location& location, 
	                        const std::string& content, size_t& pos);
	void skipWhitespace(const std::string& content, size_t& pos);
	std::string getNextToken(const std::string& content, size_t& pos);
	bool isValidPort(int port);
};

#endif
