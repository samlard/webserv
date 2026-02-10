#include "../includes/CGI.hpp"
#include "../includes/Utils.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdlib>

#define CGI_TIMEOUT 10

CGI::CGI() {
}

CGI::~CGI() {
}

bool CGI::execute(const Request& request, Response& response, 
                  const ServerConfig::Location& location, const std::string& scriptPath) {
	
	if (!Utils::fileExists(scriptPath) || !Utils::isReadable(scriptPath)) {
		response.setStatusCode(404);
		response.setBody("CGI script not found");
		return false;
	}
	
	// Build environment
	char** envp = buildEnvp(request, location, scriptPath);
	
	// Execute CGI
	std::string output;
	bool success = executeCGI(scriptPath, envp, request.getBody(), output);
	
	freeEnvp(envp);
	
	if (!success) {
		response.setStatusCode(500);
		response.setBody("CGI execution failed");
		return false;
	}
	
	// Parse CGI output
	parseCGIOutput(output, response);
	return true;
}

bool CGI::isCGIRequest(const std::string& uri, const ServerConfig::Location& location) {
	if (!location.cgiEnabled)
		return false;
	
	return Utils::endsWith(uri, location.cgiExtension);
}

char** CGI::buildEnvp(const Request& request, const ServerConfig::Location& /* location */, 
                      const std::string& scriptPath) {
	std::vector<std::string> env;
	
	// CGI environment variables
	env.push_back("REQUEST_METHOD=" + request.getMethod());
	env.push_back("SCRIPT_FILENAME=" + scriptPath);
	env.push_back("QUERY_STRING=");
	env.push_back("SERVER_PROTOCOL=" + request.getHttpVersion());
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_SOFTWARE=webserv/1.0");
	
	// Parse query string from URI
	std::string uri = request.getUri();
	size_t qPos = uri.find('?');
	if (qPos != std::string::npos) {
		env[2] = "QUERY_STRING=" + uri.substr(qPos + 1);
		env.push_back("PATH_INFO=" + uri.substr(0, qPos));
	} else {
		env.push_back("PATH_INFO=" + uri);
	}
	
	// Content length and type
	const std::map<std::string, std::string>& headers = request.getHeaders();
	std::map<std::string, std::string>::const_iterator it;
	
	it = headers.find("Content-Length");
	if (it != headers.end())
		env.push_back("CONTENT_LENGTH=" + it->second);
	
	it = headers.find("Content-Type");
	if (it != headers.end())
		env.push_back("CONTENT_TYPE=" + it->second);
	
	// Convert headers to HTTP_* variables
	for (it = headers.begin(); it != headers.end(); ++it) {
		std::string key = "HTTP_" + Utils::toUpperCase(it->first);
		// Replace - with _
		for (size_t i = 0; i < key.length(); i++) {
			if (key[i] == '-')
				key[i] = '_';
		}
		env.push_back(key + "=" + it->second);
	}
	
	// Convert to char**
	char** envp = new char*[env.size() + 1];
	for (size_t i = 0; i < env.size(); i++) {
		envp[i] = new char[env[i].length() + 1];
		std::strcpy(envp[i], env[i].c_str());
	}
	envp[env.size()] = NULL;
	
	return envp;
}

void CGI::freeEnvp(char** envp) {
	if (!envp)
		return;
	
	for (int i = 0; envp[i]; i++)
		delete[] envp[i];
	delete[] envp;
}

bool CGI::executeCGI(const std::string& scriptPath, char** envp, 
                     const std::string& body, std::string& output) {
	int pipeIn[2];
	int pipeOut[2];
	
	if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1)
		return false;
	
	pid_t pid = fork();
	if (pid == -1) {
		close(pipeIn[0]);
		close(pipeIn[1]);
		close(pipeOut[0]);
		close(pipeOut[1]);
		return false;
	}
	
	if (pid == 0) {
		// Child process
		close(pipeIn[1]);
		close(pipeOut[0]);
		
		dup2(pipeIn[0], STDIN_FILENO);
		dup2(pipeOut[1], STDOUT_FILENO);
		
		close(pipeIn[0]);
		close(pipeOut[1]);
		
		// Get interpreter from cgiPath or use the script itself
		char* argv[2];
		argv[0] = const_cast<char*>(scriptPath.c_str());
		argv[1] = NULL;
		
		execve(scriptPath.c_str(), argv, envp);
		exit(1);
	}
	
	// Parent process
	close(pipeIn[0]);
	close(pipeOut[1]);
	
	// Write body to CGI stdin
	if (!body.empty())
		write(pipeIn[1], body.c_str(), body.length());
	close(pipeIn[1]);
	
	// Read CGI output
	char buffer[4096];
	ssize_t bytesRead;
	while ((bytesRead = read(pipeOut[0], buffer, sizeof(buffer))) > 0)
		output.append(buffer, bytesRead);
	close(pipeOut[0]);
	
	// Wait for CGI to finish
	int status;
	waitpid(pid, &status, 0);
	
	return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

void CGI::parseCGIOutput(const std::string& output, Response& response) {
	size_t headerEnd = output.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		headerEnd = output.find("\n\n");
	
	if (headerEnd == std::string::npos) {
		// No headers, treat entire output as body
		response.setBody(output);
		response.setHeader("Content-Type", "text/html");
		return;
	}
	
	// Parse headers
	std::string headerSection = output.substr(0, headerEnd);
	std::string body = output.substr(headerEnd + 2);
	
	std::vector<std::string> lines = Utils::split(headerSection, '\n');
	for (size_t i = 0; i < lines.size(); i++) {
		size_t colonPos = lines[i].find(':');
		if (colonPos != std::string::npos) {
			std::string key = Utils::trim(lines[i].substr(0, colonPos));
			std::string value = Utils::trim(lines[i].substr(colonPos + 1));
			
			if (key == "Status") {
				int statusCode = Utils::stringToInt(value);
				if (statusCode > 0)
					response.setStatusCode(statusCode);
			} else {
				response.setHeader(key, value);
			}
		}
	}
	
	response.setBody(body);
}
