#include "Request.hpp"
#include <cstring>
#include "../includes/Server.hpp"

void Request::clear() {
    method.clear();
    uri.clear();
    version.clear();
    headers.clear();
    body.clear();
}


bool Server::isRequestComplete(const std::string& buffer)
{
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;

    size_t contentLenPos = buffer.find("Content-Length:");
    if (contentLenPos == std::string::npos || contentLenPos > headerEnd)
        return true;

    size_t valueStart = contentLenPos + strlen("Content-Length:");
    while (valueStart < headerEnd && (buffer[valueStart] == ' ' || buffer[valueStart] == '\t'))
        ++valueStart;

    size_t valueEnd = buffer.find("\r\n", valueStart);
    if (valueEnd == std::string::npos || valueEnd > headerEnd)
        return false;

    std::string value = buffer.substr(valueStart, valueEnd - valueStart);
    size_t expectedBodySize = static_cast<size_t>(atoi(value.c_str()));
    size_t actualBodySize = buffer.size() - (headerEnd + 4);

    return actualBodySize >= expectedBodySize;
}

void Server::parseRequest(Client& client)
{
    std::string& raw = client.getBuffer();
    Request& req = client.getRequest();

    std::istringstream stream(raw);
    std::string line;

    // Request line
    std::getline(stream, line);

    std::istringstream requestLine(line);
    requestLine >> req.method >> req.uri >> req.version;

    // Headers
    while (std::getline(stream, line) && line != "\r") {

        size_t pos = line.find(":");

        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 2);

        if (!value.empty() && value[value.size()-1] == '\r')
            value.erase(value.size()-1);

        req.headers[key] = value;
    }

    size_t bodyPos = raw.find("\r\n\r\n");
    if (bodyPos != std::string::npos)
        req.body = raw.substr(bodyPos + 4);
}