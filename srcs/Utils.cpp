#include "Utils.hpp"
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <ctime>

std::string getMimeType(const std::string& path)
{
    size_t dot = path.rfind('.');
    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dot);
    if (ext == ".html" || ext == ".htm") return "text/html; charset=UTF-8";
    if (ext == ".css")  return "text/css";
    if (ext == ".js")   return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".png")  return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif")  return "image/gif";
    if (ext == ".ico")  return "image/x-icon";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".txt")  return "text/plain";
    if (ext == ".pdf")  return "application/pdf";
    if (ext == ".xml")  return "application/xml";
    return "application/octet-stream";
}

std::string getStatusText(int code)
{
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Content Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 505: return "HTTP Version Not Supported";
        default:  return "Unknown";
    }
}

std::string intToString(int n)
{
    std::ostringstream ss;
    ss << n;
    return ss.str();
}

std::string generateDirectoryListing(const std::string& dirPath, const std::string& uri)
{
    DIR* dir = opendir(dirPath.c_str());
    if (!dir)
        return "";

    std::ostringstream html;
    html << "<!DOCTYPE html><html><head><title>Index of " << uri << "</title></head>\n";
    html << "<body><h1>Index of " << uri << "</h1><hr><pre>\n";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == ".")
            continue;

        std::string fullPath = dirPath + "/" + name;
        struct stat st;
        std::string size = "-";
        std::string mtime = "-";
        bool isDirectory = false;

        if (stat(fullPath.c_str(), &st) == 0)
        {
            isDirectory = S_ISDIR(st.st_mode);
            if (isDirectory)
                name += "/";
            else
            {
                std::ostringstream ss;
                ss << st.st_size;
                size = ss.str();
            }
            char buf[64];
            struct tm* tm_info = localtime(&st.st_mtime);
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm_info);
            mtime = buf;
        }

        html << "<a href=\"" << uri;
        if (uri.empty() || uri[uri.size()-1] != '/')
            html << "/";
        html << entry->d_name;
        if (isDirectory)
            html << "/";
        html << "\">" << name << "</a>";
        html << "\t\t" << mtime << "\t" << size << "\n";
    }
    closedir(dir);

    html << "</pre><hr></body></html>\n";
    return html.str();
}
