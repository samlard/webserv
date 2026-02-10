#include "../includes/Request.hpp"
#include "../includes/Utils.hpp"
#include <sstream>

Request::Request() 
	: _contentLength(0), _state(PARSING_REQUEST_LINE) {
}

Request::~Request() {
}

void Request::parseData(const std::string& data) {
	if (_state == COMPLETE || _state == ERROR)
		return;
	
	_buffer += data;
	
	// Parse request line
	if (_state == PARSING_REQUEST_LINE) {
		size_t pos = _buffer.find("\r\n");
		if (pos != std::string::npos) {
			parseRequestLine(_buffer.substr(0, pos));
			_buffer = _buffer.substr(pos + 2);
			if (_state != ERROR)
				_state = PARSING_HEADERS;
		}
	}
	
	// Parse headers
	if (_state == PARSING_HEADERS) {
		while (true) {
			size_t pos = _buffer.find("\r\n");
			if (pos == std::string::npos)
				break;
			
			std::string line = _buffer.substr(0, pos);
			_buffer = _buffer.substr(pos + 2);
			
			if (line.empty()) {
				// End of headers
				_contentLength = 0;
				if (_headers.find("Content-Length") != _headers.end())
					_contentLength = Utils::stringToInt(_headers["Content-Length"]);
				
				if (_contentLength > 0)
					_state = PARSING_BODY;
				else
					_state = COMPLETE;
				break;
			}
			
			parseHeader(line);
			if (_state == ERROR)
				break;
		}
	}
	
	// Parse body
	if (_state == PARSING_BODY) {
		_body += _buffer;
		_buffer.clear();
		
		if (_body.length() >= _contentLength)
			_state = COMPLETE;
	}
}

void Request::parseRequestLine(const std::string& line) {
	std::istringstream iss(line);
	iss >> _method >> _uri >> _httpVersion;
	
	if (_method.empty() || _uri.empty() || _httpVersion.empty()) {
		setError("Invalid request line");
		return;
	}
	
	if (!isValidMethod(_method)) {
		setError("Invalid HTTP method");
		return;
	}
	
	if (_httpVersion != "HTTP/1.1" && _httpVersion != "HTTP/1.0") {
		setError("Unsupported HTTP version");
		return;
	}
}

void Request::parseHeader(const std::string& line) {
	size_t pos = line.find(':');
	if (pos == std::string::npos) {
		setError("Invalid header format");
		return;
	}
	
	std::string key = Utils::trim(line.substr(0, pos));
	std::string value = Utils::trim(line.substr(pos + 1));
	
	_headers[key] = value;
}

void Request::parseBody() {
	// Body is accumulated in parseData
}

bool Request::isValidMethod(const std::string& method) const {
	return method == "GET" || method == "POST" || method == "DELETE" ||
	       method == "HEAD" || method == "PUT";
}

void Request::setState(State state) {
	_state = state;
}

void Request::setError(const std::string& message) {
	_state = ERROR;
	_errorMessage = message;
}

const std::string& Request::getMethod() const {
	return _method;
}

const std::string& Request::getUri() const {
	return _uri;
}

const std::string& Request::getHttpVersion() const {
	return _httpVersion;
}

const std::map<std::string, std::string>& Request::getHeaders() const {
	return _headers;
}

const std::string& Request::getBody() const {
	return _body;
}

Request::State Request::getState() const {
	return _state;
}

const std::string& Request::getErrorMessage() const {
	return _errorMessage;
}

bool Request::isComplete() const {
	return _state == COMPLETE;
}

bool Request::hasError() const {
	return _state == ERROR;
}

void Request::reset() {
	_method.clear();
	_uri.clear();
	_httpVersion.clear();
	_headers.clear();
	_body.clear();
	_buffer.clear();
	_errorMessage.clear();
	_contentLength = 0;
	_state = PARSING_REQUEST_LINE;
}
