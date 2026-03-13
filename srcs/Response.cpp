#include "Response.hpp"
#include <sstream>
#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

static const char* getReasonPhrase(int statusCode)
{
    switch (statusCode)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Content Too Large";
        case 500: return "Internal Server Error";
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
        return res;
    }

    if (!isMethodAllowed(loc, req.method))
    {
        Response res;
        res.statusCode = 405;
        res.body = "Method Not Allowed";
        finalizeResponseHeaders(res);
        return res;
    }

    if (config.client_max_body_size > 0 && req.body.size() > config.client_max_body_size)
    {
        Response res;
        res.statusCode = 413;
        res.body = "Payload Too Large";
        finalizeResponseHeaders(res);
        return res;
    }

    Response res;

    if (req.method == "GET")
        res = handleGet(req, config, loc);

    else if (req.method == "POST")
        res = handlePost(req, config, loc);

    else if (req.method == "DELETE")
        res = handleDelete(req, config, loc);

    else
    {
        res.statusCode = 405;
        res.body = "Method Not Allowed";
    }

    finalizeResponseHeaders(res);
    return res;
}