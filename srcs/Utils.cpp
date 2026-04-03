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

static std::string joinPath(const std::string& base, const std::string& tail)
{
    if (base.empty())
        return tail;

    if (tail.empty())
        return base;

    if (base[base.size() - 1] == '/' && tail[0] == '/')
        return base.substr(0, base.size() - 1) + tail;

    if (base[base.size() - 1] != '/' && tail[0] != '/')
        return base + "/" + tail;

    return base + tail;
}

std::string buildPathFromLocation(const std::string& uriPath, const std::string& root, const Location* loc)
{
    if (loc && !loc->root.empty() && !loc->path.empty() && loc->path != "/")
    {
        bool boundaryMatch = (uriPath == loc->path);
        if (!boundaryMatch && uriPath.size() > loc->path.size() && uriPath.find(loc->path) == 0 && uriPath[loc->path.size()] == '/')
            boundaryMatch = true;

        if (boundaryMatch)
        {
            std::string suffix = uriPath.substr(loc->path.size());
            if (suffix.empty())
                suffix = "/";
            return joinPath(root, suffix);
        }
    }

    return joinPath(root, uriPath);
}

std::string getMimeType(const std::string& path)
{
    size_t dot = path.rfind('.');
    if (dot == std::string::npos)
        return "application/octet-stream";
    std::string ext = path.substr(dot);
    if (ext == ".html" || ext == ".htm") return "text/html; charset=UTF-8";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".txt") return "text/plain";
    if (ext == ".xml") return "application/xml";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".woff") return "font/woff";
    if (ext == ".woff2") return "font/woff2";
    if (ext == ".ttf") return "font/ttf";
    if (ext == ".mp4") return "video/mp4";
    if (ext == ".webp") return "image/webp";
    return "application/octet-stream";
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