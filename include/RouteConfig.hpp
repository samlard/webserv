#ifndef ROUTECONFIG_HPP
#define ROUTECONFIG_HPP

#include <string>
#include <vector>
#include <map>

class RouteConfig {
public:
    RouteConfig();
    RouteConfig(const RouteConfig& other);
    RouteConfig& operator=(const RouteConfig& other);
    ~RouteConfig();

    // Getters
    const std::string& getPath() const;
    const std::vector<std::string>& getAllowedMethods() const;
    const std::string& getRoot() const;
    bool getAutoindex() const;
    const std::string& getIndex() const;
    const std::string& getUploadPath() const;
    const std::string& getRedirection() const;
    const std::map<std::string, std::string>& getCgiExtensions() const;

    // Setters
    void setPath(const std::string& path);
    void addAllowedMethod(const std::string& method);
    void setRoot(const std::string& root);
    void setAutoindex(bool autoindex);
    void setIndex(const std::string& index);
    void setUploadPath(const std::string& uploadPath);
    void setRedirection(const std::string& redirection);
    void addCgiExtension(const std::string& extension, const std::string& cgiPath);

private:
    std::string _path;
    std::vector<std::string> _allowedMethods;
    std::string _root;
    bool _autoindex;
    std::string _index;
    std::string _uploadPath;
    std::string _redirection;
    std::map<std::string, std::string> _cgiExtensions;
};

#endif
