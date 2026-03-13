#include "../includes/Utils.hpp"

void finalizeResponseHeaders(Response& res)
{
    if (res.headers.find("Content-Type") == res.headers.end())
        res.headers["Content-Type"] = "text/html; charset=UTF-8";

    if (res.headers.find("Content-Length") == res.headers.end())
    {
        std::stringstream ss;
        ss << res.body.size();
        res.headers["Content-Length"] = ss.str();
    }
}
bool isMethodAllowed(const Location* loc, const std::string& method)
{
    if (!loc || loc->methods.empty())
        return true;

    for (size_t i = 0; i < loc->methods.size(); ++i)
    {
        if (loc->methods[i] == method)
            return true;
    }
    return false;
}

std::string stripUriSuffix(const std::string& uri)
{
    size_t pos = uri.find_first_of("?#");
    if (pos == std::string::npos)
        return uri;
    return uri.substr(0, pos);
}

std::string buildPathFromLocation(const std::string& uriPath, const std::string& root, const Location* loc)
{
    if (!loc || loc->path.empty() || loc->path == "/")
        return root + uriPath;

    if (uriPath.find(loc->path) == 0)
    {
        std::string suffix = uriPath.substr(loc->path.size());
        if (suffix.empty())
            suffix = "/";
        else if (suffix[0] != '/')
            suffix = "/" + suffix;
        return root + suffix;
    }

    return root + uriPath;
}

std::string generateAutoindexBody(const std::string& uri, const std::string& fsPath)
{
    DIR* dir = opendir(fsPath.c_str());
    if (!dir)
        return "";

    std::stringstream body;
    body << "<html><body><h1>Index of " << uri << "</h1><ul>";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;

        body << "<li><a href=\"" << uri;
        if (!uri.empty() && uri[uri.size() - 1] != '/')
            body << "/";
        body << name << "\">" << name << "</a></li>";
    }

    body << "</ul></body></html>";
    closedir(dir);
    return body.str();
}