#include "../includes/Config.hpp"
#include "../includes/Utils.hpp"
#include <fstream>
#include <sstream>

Config::Config() {
}

Config::~Config() {
}

bool Config::parseFile(const std::string& filepath) {
	std::string content = Utils::readFile(filepath);
	if (content.empty()) {
		_error = "Failed to read configuration file";
		return false;
	}
	
	size_t pos = 0;
	while (pos < content.length()) {
		skipWhitespace(content, pos);
		if (pos >= content.length())
			break;
		
		std::string token = getNextToken(content, pos);
		if (token == "server") {
			skipWhitespace(content, pos);
			if (pos < content.length() && content[pos] == '{') {
				pos++;
				if (!parseServerBlock(content, pos)) {
					return false;
				}
			}
		} else if (token[0] == '#') {
			// Comment - skip to end of line
			while (pos < content.length() && content[pos] != '\n')
				pos++;
		}
	}
	
	if (_servers.empty()) {
		_error = "No server blocks found in configuration";
		return false;
	}
	
	return true;
}

bool Config::parseServerBlock(const std::string& content, size_t& pos) {
	ServerConfig server;
	
	while (pos < content.length()) {
		skipWhitespace(content, pos);
		if (pos >= content.length())
			break;
		
		if (content[pos] == '}') {
			pos++;
			break;
		}
		
		std::string directive = getNextToken(content, pos);
		skipWhitespace(content, pos);
		
		if (directive == "listen") {
			std::string portStr = getNextToken(content, pos);
			int port = Utils::stringToInt(portStr);
			if (!isValidPort(port)) {
				_error = "Invalid port: " + portStr;
				return false;
			}
			server.ports.push_back(port);
		}
		else if (directive == "server_name") {
			server.serverName = getNextToken(content, pos);
		}
		else if (directive == "root") {
			server.root = getNextToken(content, pos);
		}
		else if (directive == "index") {
			server.index = getNextToken(content, pos);
		}
		else if (directive == "client_max_body_size") {
			std::string sizeStr = getNextToken(content, pos);
			server.clientMaxBodySize = Utils::stringToInt(sizeStr);
		}
		else if (directive == "error_page") {
			std::string codeStr = getNextToken(content, pos);
			skipWhitespace(content, pos);
			std::string path = getNextToken(content, pos);
			int code = Utils::stringToInt(codeStr);
			server.errorPages[code] = path;
		}
		else if (directive == "location") {
			std::string path = getNextToken(content, pos);
			skipWhitespace(content, pos);
			if (pos < content.length() && content[pos] == '{') {
				pos++;
				ServerConfig::Location location;
				location.path = path;
				if (!parseLocationBlock(server, location, content, pos))
					return false;
			}
		}
		
		// Skip to semicolon or newline
		while (pos < content.length() && content[pos] != ';' && content[pos] != '\n')
			pos++;
		if (pos < content.length() && content[pos] == ';')
			pos++;
	}
	
	_servers.push_back(server);
	return true;
}

bool Config::parseLocationBlock(ServerConfig& server, ServerConfig::Location& location, 
                                 const std::string& content, size_t& pos) {
	
	// Get the path from before the '{'
	size_t pathStart = pos;
	while (pathStart > 0 && content[pathStart - 1] != ' ' && content[pathStart - 1] != '\t')
		pathStart--;
	
	while (pos < content.length()) {
		skipWhitespace(content, pos);
		if (pos >= content.length())
			break;
		
		if (content[pos] == '}') {
			pos++;
			break;
		}
		
		std::string directive = getNextToken(content, pos);
		skipWhitespace(content, pos);
		
		if (directive == "root") {
			location.root = getNextToken(content, pos);
		}
		else if (directive == "index") {
			location.index = getNextToken(content, pos);
		}
		else if (directive == "autoindex") {
			std::string value = getNextToken(content, pos);
			location.autoindex = (value == "on");
		}
		else if (directive == "allow_methods") {
			while (pos < content.length()) {
				skipWhitespace(content, pos);
				if (content[pos] == ';')
					break;
				std::string method = getNextToken(content, pos);
				if (!method.empty())
					location.allowedMethods.push_back(method);
			}
		}
		else if (directive == "return") {
			location.redirect = getNextToken(content, pos);
		}
		else if (directive == "upload_path") {
			location.uploadPath = getNextToken(content, pos);
		}
		else if (directive == "cgi_extension") {
			location.cgiExtension = getNextToken(content, pos);
			location.cgiEnabled = true;
		}
		else if (directive == "cgi_path") {
			location.cgiPath = getNextToken(content, pos);
		}
		
		// Skip to semicolon or newline
		while (pos < content.length() && content[pos] != ';' && content[pos] != '\n')
			pos++;
		if (pos < content.length() && content[pos] == ';')
			pos++;
	}
	
	server.locations.push_back(location);
	return true;
}

void Config::skipWhitespace(const std::string& content, size_t& pos) {
	while (pos < content.length() && std::isspace(content[pos]))
		pos++;
}

std::string Config::getNextToken(const std::string& content, size_t& pos) {
	skipWhitespace(content, pos);
	
	if (pos >= content.length())
		return "";
	
	size_t start = pos;
	
	// Handle comments
	if (content[pos] == '#') {
		while (pos < content.length() && content[pos] != '\n')
			pos++;
		return "#";
	}
	
	// Handle quoted strings
	if (content[pos] == '"' || content[pos] == '\'') {
		char quote = content[pos];
		pos++;
		start = pos;
		while (pos < content.length() && content[pos] != quote)
			pos++;
		std::string token = content.substr(start, pos - start);
		if (pos < content.length())
			pos++;
		return token;
	}
	
	// Handle regular tokens
	while (pos < content.length() && 
	       !std::isspace(content[pos]) && 
	       content[pos] != ';' && 
	       content[pos] != '{' && 
	       content[pos] != '}')
		pos++;
	
	return content.substr(start, pos - start);
}

bool Config::isValidPort(int port) {
	return port > 0 && port <= 65535;
}

const std::vector<ServerConfig>& Config::getServers() const {
	return _servers;
}

const std::string& Config::getError() const {
	return _error;
}

const ServerConfig::Location* ServerConfig::findLocation(const std::string& uri) const {
	const Location* bestMatch = NULL;
	size_t bestMatchLen = 0;
	
	for (size_t i = 0; i < locations.size(); i++) {
		const std::string& path = locations[i].path;
		if (Utils::startsWith(uri, path) && path.length() > bestMatchLen) {
			bestMatch = &locations[i];
			bestMatchLen = path.length();
		}
	}
	
	return bestMatch;
}
