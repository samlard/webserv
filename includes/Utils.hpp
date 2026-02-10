#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

/**
 * Utils - Helper functions
 * 
 * Responsibilities:
 * - String manipulation (trim, split, etc.)
 * - File operations (read, write, check existence)
 * - Path utilities (join, normalize)
 * - URL encoding/decoding
 * - MIME type detection
 * - Time/date formatting for HTTP headers
 */
namespace Utils {
	// String utilities
	std::string trim(const std::string& str);
	std::vector<std::string> split(const std::string& str, char delim);
	std::string toLowerCase(const std::string& str);
	std::string toUpperCase(const std::string& str);
	bool startsWith(const std::string& str, const std::string& prefix);
	bool endsWith(const std::string& str, const std::string& suffix);
	
	// File utilities
	bool fileExists(const std::string& path);
	bool isDirectory(const std::string& path);
	bool isReadable(const std::string& path);
	std::string readFile(const std::string& path);
	bool writeFile(const std::string& path, const std::string& content);
	bool deleteFile(const std::string& path);
	std::vector<std::string> listDirectory(const std::string& path);
	
	// Path utilities
	std::string joinPath(const std::string& base, const std::string& path);
	std::string normalizePath(const std::string& path);
	std::string getExtension(const std::string& path);
	
	// URL utilities
	std::string urlDecode(const std::string& str);
	std::string urlEncode(const std::string& str);
	
	// MIME type
	std::string getMimeType(const std::string& path);
	
	// HTTP utilities
	std::string getHttpDate();
	std::string intToString(int n);
	int stringToInt(const std::string& str);
	
	// Directory listing HTML
	std::string generateDirectoryListing(const std::string& dirPath, const std::string& uri);
}

#endif
