#include "../include/RouteConfig.hpp"

RouteConfig::RouteConfig() : _path("/"), _autoindex(false) {}

RouteConfig::RouteConfig(const RouteConfig& other) :
    _path(other._path),
    _allowedMethods(other._allowedMethods),
    _root(other._root),
    _autoindex(other._autoindex),
    _index(other._index),
    _uploadPath(other._uploadPath),
    _redirection(other._redirection),
    _cgiExtensions(other._cgiExtensions) {}

RouteConfig& RouteConfig::operator=(const RouteConfig& other) {
    if (this != &other) {
        _path = other._path;
        _allowedMethods = other._allowedMethods;
        _root = other._root;
        _autoindex = other._autoindex;
        _index = other._index;
        _uploadPath = other._uploadPath;
        _redirection = other._redirection;
        _cgiExtensions = other._cgiExtensions;
    }
    return *this;
}

RouteConfig::~RouteConfig() {}

const std::string& RouteConfig::getPath() const { return _path; }
const std::vector<std::string>& RouteConfig::getAllowedMethods() const { return _allowedMethods; }
const std::string& RouteConfig::getRoot() const { return _root; }
bool RouteConfig::getAutoindex() const { return _autoindex; }
const std::string& RouteConfig::getIndex() const { return _index; }
const std::string& RouteConfig::getUploadPath() const { return _uploadPath; }
const std::string& RouteConfig::getRedirection() const { return _redirection; }
const std::map<std::string, std::string>& RouteConfig::getCgiExtensions() const { return _cgiExtensions; }

void RouteConfig::setPath(const std::string& path) { _path = path; }
void RouteConfig::addAllowedMethod(const std::string& method) { _allowedMethods.push_back(method); }
void RouteConfig::setRoot(const std::string& root) { _root = root; }
void RouteConfig::setAutoindex(bool autoindex) { _autoindex = autoindex; }
void RouteConfig::setIndex(const std::string& index) { _index = index; }
void RouteConfig::setUploadPath(const std::string& uploadPath) { _uploadPath = uploadPath; }
void RouteConfig::setRedirection(const std::string& redirection) { _redirection = redirection; }
void RouteConfig::addCgiExtension(const std::string& extension, const std::string& cgiPath) {
    _cgiExtensions[extension] = cgiPath;
}
