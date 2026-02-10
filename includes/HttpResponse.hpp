#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include <sstream>

// HTTP response builder
class HttpResponse {
private:
	int _status_code;
	std::string _status_message;
	std::map<std::string, std::string> _headers;
	std::string _body;
	std::string _response_string;
	bool _is_built;

	void buildResponse();
	std::string getStatusMessage(int code);

public:
	HttpResponse();
	~HttpResponse();

	// Setters
	void setStatus(int code);
	void setHeader(const std::string& name, const std::string& value);
	void setBody(const std::string& body);
	
	// Build and get response string
	const std::string& getResponseString();
	
	// Helper to create common responses
	static HttpResponse error(int code, const std::string& message);
	static HttpResponse ok(const std::string& content_type, const std::string& body);
	
	// Reset
	void reset();
};

#endif
