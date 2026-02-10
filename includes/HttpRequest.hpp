#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <vector>

// HTTP request parser
class HttpRequest {
public:
	enum Method {
		GET,
		POST,
		DELETE,
		UNKNOWN
	};

	enum ParseState {
		REQUEST_LINE,
		HEADERS,
		BODY,
		COMPLETE,
		ERROR_STATE
	};

private:
	Method _method;
	std::string _uri;
	std::string _http_version;
	std::map<std::string, std::string> _headers;
	std::string _body;
	std::string _raw_request;
	ParseState _state;
	size_t _content_length;
	size_t _body_received;

	void parseRequestLine(const std::string& line);
	void parseHeader(const std::string& line);
	std::string trim(const std::string& str);

public:
	HttpRequest();
	~HttpRequest();

	// Parse incoming data (returns true if complete)
	bool parse(const std::string& data);
	
	// Getters
	Method getMethod() const;
	const std::string& getUri() const;
	const std::string& getHttpVersion() const;
	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getBody() const;
	ParseState getState() const;
	
	// Helper to get header value
	std::string getHeader(const std::string& name) const;
	
	// Check if request is complete
	bool isComplete() const;
	bool hasError() const;
	
	// Reset for reuse
	void reset();
};

#endif
