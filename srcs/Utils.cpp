#include "../includes/Utils.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <ctime>

namespace Utils {

// String utilities
std::string trim(const std::string& str) {
	size_t start = 0;
	size_t end = str.length();
	
	while (start < end && std::isspace(str[start]))
		start++;
	while (end > start && std::isspace(str[end - 1]))
		end--;
	
	return str.substr(start, end - start);
}

std::vector<std::string> split(const std::string& str, char delim) {
	std::vector<std::string> result;
	std::stringstream ss(str);
	std::string item;
	
	while (std::getline(ss, item, delim))
		result.push_back(item);
	
	return result;
}

std::string toLowerCase(const std::string& str) {
	std::string result = str;
	for (size_t i = 0; i < result.length(); i++)
		result[i] = std::tolower(result[i]);
	return result;
}

std::string toUpperCase(const std::string& str) {
	std::string result = str;
	for (size_t i = 0; i < result.length(); i++)
		result[i] = std::toupper(result[i]);
	return result;
}

bool startsWith(const std::string& str, const std::string& prefix) {
	return str.length() >= prefix.length() && 
	       str.compare(0, prefix.length(), prefix) == 0;
}

bool endsWith(const std::string& str, const std::string& suffix) {
	return str.length() >= suffix.length() && 
	       str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

// File utilities
bool fileExists(const std::string& path) {
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}

bool isDirectory(const std::string& path) {
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;
	return S_ISDIR(st.st_mode);
}

bool isReadable(const std::string& path) {
	return access(path.c_str(), R_OK) == 0;
}

std::string readFile(const std::string& path) {
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return "";
	
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

bool writeFile(const std::string& path, const std::string& content) {
	std::ofstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return false;
	
	file << content;
	return file.good();
}

bool deleteFile(const std::string& path) {
	return unlink(path.c_str()) == 0;
}

std::vector<std::string> listDirectory(const std::string& path) {
	std::vector<std::string> result;
	DIR* dir = opendir(path.c_str());
	if (!dir)
		return result;
	
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name != "." && name != "..")
			result.push_back(name);
	}
	closedir(dir);
	return result;
}

// Path utilities
std::string joinPath(const std::string& base, const std::string& path) {
	if (base.empty())
		return path;
	if (path.empty())
		return base;
	
	std::string result = base;
	if (result[result.length() - 1] != '/')
		result += '/';
	
	std::string cleanPath = path;
	if (cleanPath[0] == '/')
		cleanPath = cleanPath.substr(1);
	
	result += cleanPath;
	return result;
}

std::string normalizePath(const std::string& path) {
	// Handle relative paths - don't normalize if it starts with ./
	if (Utils::startsWith(path, "./"))
		return path;
	
	// For absolute paths, normalize
	if (path.empty() || path[0] != '/')
		return path;
	
	std::vector<std::string> parts = split(path, '/');
	std::vector<std::string> result;
	
	for (size_t i = 0; i < parts.size(); i++) {
		if (parts[i] == "." || parts[i].empty())
			continue;
		if (parts[i] == "..") {
			if (!result.empty())
				result.pop_back();
		} else {
			result.push_back(parts[i]);
		}
	}
	
	std::string normalized = "/";
	for (size_t i = 0; i < result.size(); i++) {
		normalized += result[i];
		if (i < result.size() - 1)
			normalized += "/";
	}
	
	return normalized;
}

std::string getExtension(const std::string& path) {
	size_t pos = path.find_last_of('.');
	if (pos == std::string::npos)
		return "";
	return path.substr(pos);
}

// URL utilities
std::string urlDecode(const std::string& str) {
	std::string result;
	for (size_t i = 0; i < str.length(); i++) {
		if (str[i] == '%' && i + 2 < str.length()) {
			int value;
			std::stringstream ss;
			ss << std::hex << str.substr(i + 1, 2);
			ss >> value;
			result += static_cast<char>(value);
			i += 2;
		} else if (str[i] == '+') {
			result += ' ';
		} else {
			result += str[i];
		}
	}
	return result;
}

std::string urlEncode(const std::string& str) {
	std::string result;
	for (size_t i = 0; i < str.length(); i++) {
		char c = str[i];
		if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			result += c;
		} else {
			result += '%';
			result += "0123456789ABCDEF"[c / 16];
			result += "0123456789ABCDEF"[c % 16];
		}
	}
	return result;
}

// MIME type
std::string getMimeType(const std::string& path) {
	std::string ext = toLowerCase(getExtension(path));
	
	if (ext == ".html" || ext == ".htm")
		return "text/html";
	if (ext == ".css")
		return "text/css";
	if (ext == ".js")
		return "application/javascript";
	if (ext == ".json")
		return "application/json";
	if (ext == ".jpg" || ext == ".jpeg")
		return "image/jpeg";
	if (ext == ".png")
		return "image/png";
	if (ext == ".gif")
		return "image/gif";
	if (ext == ".svg")
		return "image/svg+xml";
	if (ext == ".txt")
		return "text/plain";
	if (ext == ".pdf")
		return "application/pdf";
	if (ext == ".zip")
		return "application/zip";
	
	return "application/octet-stream";
}

// HTTP utilities
std::string getHttpDate() {
	time_t now = time(NULL);
	struct tm* timeinfo = gmtime(&now);
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
	return std::string(buffer);
}

std::string intToString(int n) {
	std::stringstream ss;
	ss << n;
	return ss.str();
}

int stringToInt(const std::string& str) {
	std::stringstream ss(str);
	int n;
	ss >> n;
	return n;
}

// Directory listing HTML
std::string generateDirectoryListing(const std::string& dirPath, const std::string& uri) {
	std::vector<std::string> entries = listDirectory(dirPath);
	
	std::string html = "<!DOCTYPE html>\n<html>\n<head>\n";
	html += "<title>Index of " + uri + "</title>\n";
	html += "<style>body{font-family:monospace;margin:40px;}</style>\n";
	html += "</head>\n<body>\n";
	html += "<h1>Index of " + uri + "</h1>\n<hr>\n<ul>\n";
	
	if (uri != "/")
		html += "<li><a href=\"../\">../</a></li>\n";
	
	for (size_t i = 0; i < entries.size(); i++) {
		std::string fullPath = joinPath(dirPath, entries[i]);
		std::string link = entries[i];
		if (isDirectory(fullPath))
			link += "/";
		html += "<li><a href=\"" + urlEncode(link) + "\">" + link + "</a></li>\n";
	}
	
	html += "</ul>\n<hr>\n</body>\n</html>";
	return html;
}

} // namespace Utils
