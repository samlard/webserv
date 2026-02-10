#include "HttpRequest.hpp"
#include <sstream>
#include <algorithm>

HttpRequest::HttpRequest() : _method(UNKNOWN), _state(REQUEST_LINE), 
	_content_length(0), _body_received(0) {}

HttpRequest::~HttpRequest() {}

std::string HttpRequest::trim(const std::string& str) {
	size_t first = str.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, last - first + 1);
}

bool HttpRequest::parse(const std::string& data) {
	_raw_request += data;

	if (_state == REQUEST_LINE) {
		size_t line_end = _raw_request.find("\r\n");
		if (line_end != std::string::npos) {
			std::string line = _raw_request.substr(0, line_end);
			parseRequestLine(line);
			_raw_request.erase(0, line_end + 2);
			_state = HEADERS;
		}
	}

	if (_state == HEADERS) {
		size_t headers_end = _raw_request.find("\r\n\r\n");
		if (headers_end != std::string::npos) {
			std::string headers = _raw_request.substr(0, headers_end);
			_raw_request.erase(0, headers_end + 4);

			std::istringstream iss(headers);
			std::string line;
			while (std::getline(iss, line)) {
				if (!line.empty() && line[line.size() - 1] == '\r')
					line.erase(line.size() - 1);
				parseHeader(line);
			}

			std::string cl = getHeader("Content-Length");
			if (!cl.empty()) {
				_content_length = std::atol(cl.c_str());
				_state = BODY;
			} else {
				_state = COMPLETE;
			}
		}
	}

	if (_state == BODY) {
		_body += _raw_request;
		_body_received = _body.size();
		_raw_request.clear();

		if (_body_received >= _content_length)
			_state = COMPLETE;
	}

	return _state == COMPLETE;
}

void HttpRequest::parseRequestLine(const std::string& line) {
	std::istringstream iss(line);
	std::string method_str;
	iss >> method_str >> _uri >> _http_version;

	if (method_str == "GET")
		_method = GET;
	else if (method_str == "POST")
		_method = POST;
	else if (method_str == "DELETE")
		_method = DELETE;
	else
		_method = UNKNOWN;
}

void HttpRequest::parseHeader(const std::string& line) {
	size_t colon = line.find(':');
	if (colon != std::string::npos) {
		std::string name = trim(line.substr(0, colon));
		std::string value = trim(line.substr(colon + 1));
		_headers[name] = value;
	}
}

HttpRequest::Method HttpRequest::getMethod() const {
	return _method;
}

const std::string& HttpRequest::getUri() const {
	return _uri;
}

const std::string& HttpRequest::getHttpVersion() const {
	return _http_version;
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
	return _headers;
}

const std::string& HttpRequest::getBody() const {
	return _body;
}

HttpRequest::ParseState HttpRequest::getState() const {
	return _state;
}

std::string HttpRequest::getHeader(const std::string& name) const {
	std::map<std::string, std::string>::const_iterator it = _headers.find(name);
	if (it != _headers.end())
		return it->second;
	return "";
}

bool HttpRequest::isComplete() const {
	return _state == COMPLETE;
}

bool HttpRequest::hasError() const {
	return _state == ERROR_STATE;
}

void HttpRequest::reset() {
	_method = UNKNOWN;
	_uri.clear();
	_http_version.clear();
	_headers.clear();
	_body.clear();
	_raw_request.clear();
	_state = REQUEST_LINE;
	_content_length = 0;
	_body_received = 0;
}
