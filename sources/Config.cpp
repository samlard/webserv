#include "Config.hpp"
#include <iostream>
#include <sstream>

ServerConfig::ServerConfig() : client_max_body_size(1048576), autoindex(false) {
	index_files.push_back("index.html");
}

Config::Config() {}

Config::Config(const std::string& config_file) : _config_file(config_file) {
	load(config_file);
}

Config::~Config() {}

std::string Config::trim(const std::string& str) {
	size_t first = str.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, last - first + 1);
}

bool Config::load(const std::string& config_file) {
	std::ifstream file(config_file.c_str());
	if (!file.is_open()) {
		std::cerr << "Error: Cannot open config file: " << config_file << std::endl;
		return false;
	}

	try {
		parseConfigFile(file);
	} catch (const std::exception& e) {
		std::cerr << "Error parsing config: " << e.what() << std::endl;
		file.close();
		return false;
	}

	file.close();
	return true;
}

void Config::parseConfigFile(std::ifstream& file) {
	std::string line;
	while (std::getline(file, line)) {
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue;

		if (line == "server {") {
			ServerConfig server;
			parseServerBlock(file, server);
			_servers.push_back(server);
		}
	}
}

void Config::parseServerBlock(std::ifstream& file, ServerConfig& server) {
	std::string line;
	while (std::getline(file, line)) {
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue;

		if (line == "}")
			return;

		std::istringstream iss(line);
		std::string directive;
		iss >> directive;

		if (directive == "listen") {
			int port;
			iss >> port;
			server.ports.push_back(port);
		} else if (directive == "server_name") {
			std::string name;
			while (iss >> name)
				server.server_names.push_back(name);
		} else if (directive == "client_max_body_size") {
			iss >> server.client_max_body_size;
		} else if (directive == "error_page_404") {
			iss >> server.error_page_404;
		} else if (directive == "autoindex") {
			std::string value;
			iss >> value;
			server.autoindex = (value == "on");
		} else if (directive == "location") {
			std::string path, root_directive, root_path;
			iss >> path >> root_directive >> root_path;
			if (root_directive == "root")
				server.locations[path] = root_path;
		} else if (directive == "cgi_extension") {
			std::string ext, interpreter;
			iss >> ext >> interpreter;
			server.cgi_extensions[ext] = interpreter;
		}
	}
}

const std::vector<ServerConfig>& Config::getServers() const {
	return _servers;
}
