#include "RequestHandler.hpp"
#include "CgiHandler.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

RequestHandler::RequestHandler(const ServerConfig& config) : _config(config) {}

RequestHandler::~RequestHandler() {}

void RequestHandler::handle(const HttpRequest& request, HttpResponse& response) {
	switch (request.getMethod()) {
		case HttpRequest::GET:
			handleGet(request, response);
			break;
		case HttpRequest::POST:
			handlePost(request, response);
			break;
		case HttpRequest::DELETE:
			handleDelete(request, response);
			break;
		default:
			response = HttpResponse::error(405, "Method not allowed");
	}
}

void RequestHandler::handleGet(const HttpRequest& request, HttpResponse& response) {
	std::string path = resolvePath(request.getUri());
	
	if (!fileExists(path)) {
		response = HttpResponse::error(404, "File not found");
		return;
	}
	
	struct stat st;
	if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
		// Try index files
		for (size_t i = 0; i < _config.index_files.size(); ++i) {
			std::string index_path = path + "/" + _config.index_files[i];
			if (fileExists(index_path)) {
				path = index_path;
				break;
			}
		}
		
		// If still directory and autoindex enabled
		if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
			if (_config.autoindex) {
				std::string listing = generateDirectoryListing(path, request.getUri());
				response = HttpResponse::ok("text/html", listing);
				return;
			} else {
				response = HttpResponse::error(403, "Directory listing forbidden");
				return;
			}
		}
	}
	
	std::string content = readFile(path);
	if (content.empty()) {
		response = HttpResponse::error(500, "Cannot read file");
		return;
	}
	
	response = HttpResponse::ok(getContentType(path), content);
}

void RequestHandler::handlePost(const HttpRequest& request, HttpResponse& response) {
	std::string path = resolvePath(request.getUri());
	const std::string& body = request.getBody();
	
	if (body.size() > _config.client_max_body_size) {
		response = HttpResponse::error(413, "Payload too large");
		return;
	}
	
	if (writeFile(path, body)) {
		response.setStatus(201);
		response.setHeader("Content-Type", "text/plain");
		response.setBody("File uploaded successfully");
	} else {
		response = HttpResponse::error(500, "Cannot write file");
	}
}

void RequestHandler::handleDelete(const HttpRequest& request, HttpResponse& response) {
	std::string path = resolvePath(request.getUri());
	
	if (!fileExists(path)) {
		response = HttpResponse::error(404, "File not found");
		return;
	}
	
	if (deleteFile(path)) {
		response.setStatus(204);
	} else {
		response = HttpResponse::error(500, "Cannot delete file");
	}
}

std::string RequestHandler::getContentType(const std::string& path) {
	size_t dot = path.rfind('.');
	if (dot != std::string::npos) {
		std::string ext = path.substr(dot);
		if (ext == ".html" || ext == ".htm") return "text/html";
		if (ext == ".css") return "text/css";
		if (ext == ".js") return "application/javascript";
		if (ext == ".json") return "application/json";
		if (ext == ".png") return "image/png";
		if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
		if (ext == ".gif") return "image/gif";
		if (ext == ".txt") return "text/plain";
	}
	return "application/octet-stream";
}

bool RequestHandler::fileExists(const std::string& path) {
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}

std::string RequestHandler::readFile(const std::string& path) {
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return "";
	
	std::ostringstream oss;
	oss << file.rdbuf();
	return oss.str();
}

bool RequestHandler::writeFile(const std::string& path, const std::string& content) {
	std::ofstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return false;
	
	file << content;
	return file.good();
}

bool RequestHandler::deleteFile(const std::string& path) {
	return unlink(path.c_str()) == 0;
}

std::string RequestHandler::resolvePath(const std::string& uri) {
	// Remove query string first
	std::string path = uri;
	size_t query = path.find('?');
	if (query != std::string::npos)
		path = path.substr(0, query);
	
	// Find matching location
	std::string root = "./www";
	std::string location_prefix;
	for (std::map<std::string, std::string>::const_iterator it = _config.locations.begin();
		 it != _config.locations.end(); ++it) {
		if (path.find(it->first) == 0) {
			root = it->second;
			location_prefix = it->first;
			break;
		}
	}
	
	// Remove location prefix from path if it's not "/"
	if (!location_prefix.empty() && location_prefix != "/") {
		path = path.substr(location_prefix.length());
	}
	
	return root + path;
}

std::string RequestHandler::generateDirectoryListing(const std::string& path, const std::string& uri) {
	std::ostringstream html;
	html << "<html><head><title>Index of " << uri << "</title></head>";
	html << "<body><h1>Index of " << uri << "</h1><hr><ul>";
	
	DIR* dir = opendir(path.c_str());
	if (dir) {
		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL) {
			std::string name = entry->d_name;
			if (name == ".")
				continue;
			
			std::string link = uri;
			if (link[link.size() - 1] != '/')
				link += "/";
			link += name;
			
			html << "<li><a href=\"" << link << "\">" << name << "</a></li>";
		}
		closedir(dir);
	}
	
	html << "</ul><hr></body></html>";
	return html.str();
}

bool RequestHandler::isCgiRequest(const std::string& uri) {
	for (std::map<std::string, std::string>::const_iterator it = _config.cgi_extensions.begin();
		 it != _config.cgi_extensions.end(); ++it) {
		if (uri.find(it->first) != std::string::npos)
			return true;
	}
	return false;
}
