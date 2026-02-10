#include "CgiHandler.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdlib>

CgiHandler::CgiHandler(const ServerConfig& config) : _config(config) {}

CgiHandler::~CgiHandler() {}

void CgiHandler::handle(const HttpRequest& request, HttpResponse& response, 
						const std::string& script_path) {
	std::string output;
	if (executeCgi(script_path, request, output)) {
		// Parse CGI output (headers + body)
		size_t header_end = output.find("\r\n\r\n");
		if (header_end != std::string::npos) {
			std::string body = output.substr(header_end + 4);
			response.setStatus(200);
			response.setHeader("Content-Type", "text/html");
			response.setBody(body);
		} else {
			response.setStatus(200);
			response.setHeader("Content-Type", "text/html");
			response.setBody(output);
		}
	} else {
		response = HttpResponse::error(500, "CGI execution failed");
	}
}

bool CgiHandler::executeCgi(const std::string& script_path, 
							 const HttpRequest& request, 
							 std::string& output) {
	int pipefd[2];
	if (pipe(pipefd) < 0)
		return false;
	
	pid_t pid = fork();
	if (pid < 0) {
		close(pipefd[0]);
		close(pipefd[1]);
		return false;
	}
	
	if (pid == 0) {
		// Child process
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		
		std::string interpreter = findCgiInterpreter(script_path);
		std::vector<std::string> env = buildEnv(request, script_path);
		
		// Convert env to char* array
		char** envp = new char*[env.size() + 1];
		for (size_t i = 0; i < env.size(); ++i) {
			envp[i] = const_cast<char*>(env[i].c_str());
		}
		envp[env.size()] = NULL;
		
		// Execute CGI
		char* argv[] = {
			const_cast<char*>(interpreter.c_str()),
			const_cast<char*>(script_path.c_str()),
			NULL
		};
		
		execve(interpreter.c_str(), argv, envp);
		exit(1); // execve failed
	}
	
	// Parent process
	close(pipefd[1]);
	
	char buffer[4096];
	ssize_t bytes;
	while ((bytes = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
		output.append(buffer, bytes);
	}
	
	close(pipefd[0]);
	
	int status;
	waitpid(pid, &status, 0);
	
	return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

std::vector<std::string> CgiHandler::buildEnv(const HttpRequest& request, 
											   const std::string& script_path) {
	std::vector<std::string> env;
	
	env.push_back("REQUEST_METHOD=" + std::string(
		request.getMethod() == HttpRequest::GET ? "GET" :
		request.getMethod() == HttpRequest::POST ? "POST" : "DELETE"));
	
	env.push_back("SCRIPT_FILENAME=" + script_path);
	env.push_back("QUERY_STRING=");
	
	size_t query = request.getUri().find('?');
	if (query != std::string::npos) {
		env[env.size() - 1] = "QUERY_STRING=" + request.getUri().substr(query + 1);
	}
	
	std::string cl = request.getHeader("Content-Length");
	if (!cl.empty())
		env.push_back("CONTENT_LENGTH=" + cl);
	
	std::string ct = request.getHeader("Content-Type");
	if (!ct.empty())
		env.push_back("CONTENT_TYPE=" + ct);
	
	return env;
}

std::string CgiHandler::findCgiInterpreter(const std::string& script_path) {
	size_t dot = script_path.rfind('.');
	if (dot != std::string::npos) {
		std::string ext = script_path.substr(dot);
		std::map<std::string, std::string>::const_iterator it = _config.cgi_extensions.find(ext);
		if (it != _config.cgi_extensions.end())
			return it->second;
	}
	return "/bin/sh";
}
