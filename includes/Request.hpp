#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

/**
 * Request - Parses and holds HTTP request data
 * 
 * Responsibilities:
 * - Parse HTTP request line by line in non-blocking mode
 * - Store method, URI, HTTP version
 * - Parse and store headers
 * - Handle request body (for POST, file uploads)
 * - Validate HTTP syntax
 * - Track parsing state for incremental parsing
 */
class Request {
public:
	enum State {
		PARSING_REQUEST_LINE,
		PARSING_HEADERS,
		PARSING_BODY,
		COMPLETE,
		ERROR
	};

	Request();
	~Request();

	// Parse incoming data incrementally (non-blocking)
	void parseData(const std::string& data);
	
	// Getters
	const std::string& getMethod() const;
	const std::string& getUri() const;
	const std::string& getHttpVersion() const;
	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getBody() const;
	State getState() const;
	const std::string& getErrorMessage() const;
	
	// Check if request is complete
	bool isComplete() const;
	bool hasError() const;
	
	// Reset for reuse
	void reset();

private:
	// Request line
	std::string _method;
	std::string _uri;
	std::string _httpVersion;
	
	// Headers
	std::map<std::string, std::string> _headers;
	
	// Body
	std::string _body;
	size_t _contentLength;
	
	// Parsing state
	State _state;
	std::string _buffer;
	std::string _errorMessage;
	
	// Helper methods
	void parseRequestLine(const std::string& line);
	void parseHeader(const std::string& line);
	void parseBody();
	bool isValidMethod(const std::string& method) const;
	void setState(State state);
	void setError(const std::string& message);
};

#endif
