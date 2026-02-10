#include "../includes/Response.hpp"
#include "../includes/Utils.hpp"
#include <sstream>

Response::Response() : _statusCode(200) {
	setDefaultHeaders();
}

Response::~Response() {
}

void Response::setStatusCode(int code) {
	_statusCode = code;
}

void Response::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
}

void Response::setBody(const std::string& body) {
	_body = body;
	_headers["Content-Length"] = Utils::intToString(_body.length());
}

void Response::setBodyFromFile(const std::string& filepath) {
	_body = Utils::readFile(filepath);
	_headers["Content-Length"] = Utils::intToString(_body.length());
	_headers["Content-Type"] = Utils::getMimeType(filepath);
}

std::string Response::toString() const {
	std::stringstream ss;
	
	// Status line
	ss << "HTTP/1.1 " << _statusCode << " " << getStatusText(_statusCode) << "\r\n";
	
	// Headers
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
	     it != _headers.end(); ++it) {
		ss << it->first << ": " << it->second << "\r\n";
	}
	
	ss << "\r\n";
	
	// Body
	ss << _body;
	
	return ss.str();
}

size_t Response::getSize() const {
	return toString().length();
}

void Response::reset() {
	_statusCode = 200;
	_headers.clear();
	_body.clear();
	setDefaultHeaders();
}

void Response::setDefaultHeaders() {
	_headers["Server"] = "webserv/1.0";
	_headers["Date"] = Utils::getHttpDate();
	_headers["Connection"] = "keep-alive";
}

std::string Response::getStatusText(int code) {
	switch (code) {
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		default: return "Unknown";
	}
}
