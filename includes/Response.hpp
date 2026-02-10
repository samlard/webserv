#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

/**
 * Response - Builds HTTP response
 * 
 * Responsibilities:
 * - Build HTTP response with status code, headers, body
 * - Generate proper status line
 * - Format headers correctly
 * - Handle different content types
 * - Support chunked responses for large files
 */
class Response {
public:
	Response();
	~Response();

	// Build response
	void setStatusCode(int code);
	void setHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);
	void setBodyFromFile(const std::string& filepath);
	
	// Generate the full HTTP response string
	std::string toString() const;
	
	// Get response size
	size_t getSize() const;
	
	// Reset for reuse
	void reset();
	
	// Helper to get status text from code
	static std::string getStatusText(int code);

private:
	int _statusCode;
	std::map<std::string, std::string> _headers;
	std::string _body;
	
	void setDefaultHeaders();
};

#endif
