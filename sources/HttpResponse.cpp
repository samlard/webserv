#include "HttpResponse.hpp"

HttpResponse::HttpResponse() : _status_code(200), _status_message("OK"), _is_built(false) {}

HttpResponse::~HttpResponse() {}

void HttpResponse::setStatus(int code) {
	_status_code = code;
	_status_message = getStatusMessage(code);
	_is_built = false;
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
	_headers[name] = value;
	_is_built = false;
}

void HttpResponse::setBody(const std::string& body) {
	_body = body;
	_is_built = false;
}

std::string HttpResponse::getStatusMessage(int code) {
	switch (code) {
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		default: return "Unknown";
	}
}

void HttpResponse::buildResponse() {
	if (_is_built)
		return;

	std::ostringstream oss;
	oss << "HTTP/1.1 " << _status_code << " " << _status_message << "\r\n";

	if (_headers.find("Content-Length") == _headers.end() && !_body.empty()) {
		std::ostringstream content_length;
		content_length << _body.size();
		_headers["Content-Length"] = content_length.str();
	}

	if (_headers.find("Connection") == _headers.end())
		_headers["Connection"] = "close";

	for (std::map<std::string, std::string>::iterator it = _headers.begin();
		 it != _headers.end(); ++it) {
		oss << it->first << ": " << it->second << "\r\n";
	}

	oss << "\r\n";
	if (!_body.empty())
		oss << _body;

	_response_string = oss.str();
	_is_built = true;
}

const std::string& HttpResponse::getResponseString() {
	buildResponse();
	return _response_string;
}

HttpResponse HttpResponse::error(int code, const std::string& message) {
	HttpResponse response;
	response.setStatus(code);
	response.setHeader("Content-Type", "text/html");
	
	std::ostringstream body;
	body << "<html><body><h1>Error " << code << "</h1><p>" 
		 << message << "</p></body></html>";
	response.setBody(body.str());
	
	return response;
}

HttpResponse HttpResponse::ok(const std::string& content_type, const std::string& body) {
	HttpResponse response;
	response.setStatus(200);
	response.setHeader("Content-Type", content_type);
	response.setBody(body);
	return response;
}

void HttpResponse::reset() {
	_status_code = 200;
	_status_message = "OK";
	_headers.clear();
	_body.clear();
	_response_string.clear();
	_is_built = false;
}
