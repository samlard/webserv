#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Config.hpp"
#include <string>

// Handles HTTP requests and generates responses
class RequestHandler {
private:
	const ServerConfig& _config;

	void handleGet(const HttpRequest& request, HttpResponse& response);
	void handlePost(const HttpRequest& request, HttpResponse& response);
	void handleDelete(const HttpRequest& request, HttpResponse& response);
	
	std::string getContentType(const std::string& path);
	bool fileExists(const std::string& path);
	std::string readFile(const std::string& path);
	bool writeFile(const std::string& path, const std::string& content);
	bool deleteFile(const std::string& path);
	std::string resolvePath(const std::string& uri);
	std::string generateDirectoryListing(const std::string& path, const std::string& uri);

public:
	RequestHandler(const ServerConfig& config);
	~RequestHandler();

	void handle(const HttpRequest& request, HttpResponse& response);
	bool isCgiRequest(const std::string& uri);
};

#endif
