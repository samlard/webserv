#include "../includes/Response.hpp"
#include <sstream>
#include <fstream>
#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

static const char* getReasonPhrase(int statusCode)
{
    switch (statusCode)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 413: return "Content Too Large";
        case 414: return "URI Too Long";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default: return "OK";
    }
}

std::string Response::toString() const {

    std::stringstream ss;

    ss << "HTTP/1.1 " << statusCode << " " << getReasonPhrase(statusCode) << "\r\n";

    for (std::map<std::string,std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
        ss << it->first << ": " << it->second << "\r\n";

    ss << "\r\n";
    ss << body;

    return ss.str();
}

Response Server::applyErrorPage(Response& res, const ServerConfig& config)
{
    if (res.statusCode >= 400 && config.error_pages.count(res.statusCode))
    {
        std::string errorPagePath = config.root + config.error_pages.find(res.statusCode)->second;
        std::ifstream f(errorPagePath.c_str());
        if (f.is_open())
        {
            std::stringstream buf;
            buf << f.rdbuf();
            res.body = buf.str();
            std::stringstream ss;
            ss << res.body.size();
            res.headers["Content-Length"] = ss.str();
        }
    }
    return res;
}

Response Server::buildResponse(Client& client)
{
    Request& req = client.getRequest();
    ServerConfig& config = _serverConfigs[client.getServerIndex()];

    Location* loc = matchLocation(config, req.uri);

    if (!loc)
    {
        Response res;
        res.statusCode = 404;
        res.body = "404 Not Found";
        finalizeResponseHeaders(res);
        return applyErrorPage(res, config);
    }

    if (loc && !loc->redirect.empty())
    {
        Response res;
        std::istringstream iss(loc->redirect);
        int code;
        std::string url;
        if (iss >> code && iss >> url && code >= 300 && code <= 399)
        {
            res.statusCode = code;
            res.headers["Location"] = url;
        }
        else
        {
            res.statusCode = 301;
            res.headers["Location"] = loc->redirect;
        }
        res.body = "";
        finalizeResponseHeaders(res);
        return res;
    }

    if (!isMethodAllowed(loc, req.method))
    {
        Response res;
        res.statusCode = 405;
        res.body = "405 Method Not Allowed";
        finalizeResponseHeaders(res);
        return applyErrorPage(res, config);
    }

    if (config.client_max_body_size > 0 && req.body.size() > config.client_max_body_size)
    {
        Response res;
        res.statusCode = 413;
        res.body = "413 Payload Too Large";
        finalizeResponseHeaders(res);
        return applyErrorPage(res, config);
    }

    Response res;

    if (req.method == "GET")
        res = handleGet(req, config, loc);

    else if (req.method == "HEAD")
    {
        res = handleGet(req, config, loc);
        res.body.clear();
    }

    else if (req.method == "POST")
        res = handlePost(req, config, loc);

    else if (req.method == "DELETE")
        res = handleDelete(req, config, loc);

    else
    {
        res.statusCode = 405;
        res.body = "405 Method Not Allowed";
    }

    finalizeResponseHeaders(res);
    return applyErrorPage(res, config);
}