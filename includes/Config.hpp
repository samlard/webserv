#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <fstream>

// Represents a server block in the config
struct ServerConfig {
	std::vector<int> ports;
	std::vector<std::string> server_names;
	std::map<std::string, std::string> locations; // path -> root
	std::map<std::string, std::string> cgi_extensions; // .py -> /usr/bin/python
	size_t client_max_body_size;
	std::string error_page_404;
	std::vector<std::string> index_files;
	bool autoindex;

	ServerConfig();
};

// Main configuration parser
class Config {
private:
	std::vector<ServerConfig> _servers;
	std::string _config_file;

	void parseConfigFile(std::ifstream& file);
	void parseServerBlock(std::ifstream& file, ServerConfig& server);
	std::string trim(const std::string& str);

public:
	Config();
	Config(const std::string& config_file);
	~Config();

	bool load(const std::string& config_file);
	const std::vector<ServerConfig>& getServers() const;
};

#endif
