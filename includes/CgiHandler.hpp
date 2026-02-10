#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Config.hpp"
#include <string>
#include <vector>

// Handles CGI execution via fork+execve
class CgiHandler {
private:
	const ServerConfig& _config;

	std::vector<std::string> buildEnv(const HttpRequest& request, const std::string& script_path);
	std::string findCgiInterpreter(const std::string& script_path);
	bool executeCgi(const std::string& script_path, const HttpRequest& request, std::string& output);

public:
	CgiHandler(const ServerConfig& config);
	~CgiHandler();

	void handle(const HttpRequest& request, HttpResponse& response, const std::string& script_path);
};

#endif
