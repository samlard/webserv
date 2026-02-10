#ifndef CGI_HPP
#define CGI_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "Config.hpp"
#include <string>

/**
 * CGI - Handles CGI script execution
 * 
 * Responsibilities:
 * - Fork and execute CGI scripts
 * - Set up environment variables
 * - Pipe data to/from CGI process
 * - Handle CGI timeouts
 * - Parse CGI output
 * - Convert CGI output to HTTP response
 */
class CGI {
public:
	CGI();
	~CGI();

	// Execute CGI script
	bool execute(const Request& request, Response& response, 
	             const ServerConfig::Location& location, const std::string& scriptPath);
	
	// Check if URI should be handled by CGI
	static bool isCGIRequest(const std::string& uri, const ServerConfig::Location& location);

private:
	// Environment setup
	char** buildEnvp(const Request& request, const ServerConfig::Location& location, 
	                  const std::string& scriptPath);
	void freeEnvp(char** envp);
	
	// Execution
	bool executeCGI(const std::string& scriptPath, char** envp, 
	                const std::string& body, std::string& output);
	
	// Output parsing
	void parseCGIOutput(const std::string& output, Response& response);
};

#endif
