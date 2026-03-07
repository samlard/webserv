#include "../includes/Server.hpp"
#include <dirent.h>

#define BUFFER_SIZE 4096

Server::Server() : _running(false) {
}

Server::~Server() {
    for (size_t i = 0; i < _listenSockets.size(); i++)
        close(_listenSockets[i]);
}

void Server::shutdown(){
    _running = false;
}

static bool isRequestComplete(const std::string& buffer)
{
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;

    // Check Content-Length header to ensure body is fully received
    size_t clPos = buffer.find("Content-Length:");
    if (clPos == std::string::npos)
        clPos = buffer.find("content-length:");
    if (clPos != std::string::npos && clPos < headerEnd)
    {
        size_t valStart = buffer.find_first_not_of(" \t", clPos + 15);
        size_t valEnd = buffer.find("\r\n", valStart);
        if (valStart != std::string::npos && valEnd != std::string::npos)
        {
            std::string clStr = buffer.substr(valStart, valEnd - valStart);
            // Validate that Content-Length is a numeric value
            bool valid = !clStr.empty();
            for (size_t k = 0; k < clStr.size() && valid; k++)
                if (!isdigit((unsigned char)clStr[k]))
                    valid = false;
            if (!valid)
                return true;
            size_t contentLength = (size_t)atol(clStr.c_str());
            size_t bodyStart = headerEnd + 4;
            return buffer.size() >= bodyStart + contentLength;
        }
    }
    return true;
}

int Server::findServerIndex(int listenSocket) const {
    for (size_t i = 0; i < _listenSockets.size(); i++) {
        if (_listenSockets[i] == listenSocket) {
            return (int)i;
        }
    }
    return -1;
}

int Server::init(Config &config) {

    const std::vector<ServerConfig>& servers = config.getServers();
    
    if (servers.empty()) {
        std::cerr << "Error: No server configuration found" << std::endl;
        return 1;
    }
    
    for (size_t i = 0; i < servers.size(); i++) {
        int port = servers[i].port;
        std::string host = servers[i].host;
        
        if (port == 0) {
            std::cerr << "Error: Server " << i << " has no port" << std::endl;
            continue;
        }
        
        int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (listenSocket < 0) {
            perror("socket");
            for (size_t j = 0; j < _listenSockets.size(); j++) {
                close(_listenSockets[j]);
            }
            return 1;
        }
    
        if (fcntl(listenSocket, F_SETFL, O_NONBLOCK) < 0) {
            perror("fcntl");
            close(listenSocket);
            for (size_t j = 0; j < _listenSockets.size(); j++) {
                close(_listenSockets[j]);
            }
            return 1;
        }
        
        int opt = 1;
        if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt");
            close(listenSocket);
            for (size_t j = 0; j < _listenSockets.size(); j++) {
                close(_listenSockets[j]);
            }
            return 1;
        }
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        
        if (host.empty() || host == "0.0.0.0") {
            addr.sin_addr.s_addr = INADDR_ANY;
            host = "0.0.0.0";
        } else {
            addr.sin_addr.s_addr = inet_addr(host.c_str());
        }
        if (bind(listenSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind");
            close(listenSocket);
            for (size_t j = 0; j < _listenSockets.size(); j++) {
                close(_listenSockets[j]);
            }
            return 1;
        }
        
        if (listen(listenSocket, 128) < 0) {
            perror("listen");
            close(listenSocket);
            for (size_t j = 0; j < _listenSockets.size(); j++) {
                close(_listenSockets[j]);
            }
            return 1;
        }
        
        _listenSockets.push_back(listenSocket);
        _serverConfigs.push_back(servers[i]);
        
        pollfd pfd;
        pfd.fd = listenSocket;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _fds.push_back(pfd);
        std::cout << "  [" << i << "] Listening on " << host << ":" << port 
                  << " (fd=" << listenSocket << ")" << std::endl;
    }
    
    if (_listenSockets.empty()) {
        std::cerr << "Error: No server could be initialized" << std::endl;
        return 1;
    }
    _running = true;
    std::cout << "Server initialized successfully" << std::endl;
    return 0;
}

void Server::run()
{
    while (_running)
    {
        int ret = poll(&_fds[0], _fds.size(), -1);

        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }

        for (int i = (int)_fds.size() - 1; i >= 0; --i)
        {
            int fd = _fds[i].fd;
            short revents = _fds[i].revents;

            if (revents == 0)
                continue;

            bool isListenSocket = false;
            for (size_t j = 0; j < _listenSockets.size(); ++j)
            {
                if (_listenSockets[j] == fd)
                {
                    isListenSocket = true;
                    break;
                }
            }

            if (isListenSocket && (revents & POLLIN))
            {
                acceptNewClient(fd);
                continue;
            }

            if (!isListenSocket && (revents & POLLIN))
            {
                handleClientRead(i);
                // handleClientRead may have erased _fds[i], check before continuing
                if (i >= (int)_fds.size() || _fds[i].fd != fd)
                    continue;
            }

            if (!isListenSocket && (_fds[i].revents & POLLOUT))
            {
                Client &client = _clients[fd];
                std::string responseStr = client.getResponse().toString();

                ssize_t sent = send(fd, responseStr.c_str(), responseStr.size(), 0);
                if (sent < 0)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        close(fd);
                        _clients.erase(fd);
                        _fds.erase(_fds.begin() + i);
                    }
                    continue;
                }

                // This server currently sends one response per request then closes.
                close(fd);
                _clients.erase(fd);
                _fds.erase(_fds.begin() + i);
                continue;
            }

            if (revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                if (!isListenSocket)
                {
                    close(fd);
                    _clients.erase(fd);
                    _fds.erase(_fds.begin() + i);
                }
            }
        }
    }
}

void Server::parseRequest(Client& client)
{
    const std::string& raw = client.getBuffer();
    Request& req = client.getRequest();

    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return;

    std::istringstream stream(raw.substr(0, headerEnd));
    std::string line;

    // Request line
    std::getline(stream, line);
    if (!line.empty() && line[line.size()-1] == '\r')
        line.erase(line.size()-1);

    std::istringstream requestLine(line);
    requestLine >> req.method >> req.uri >> req.version;

    // Strip query string from URI for file path purposes (keep full URI for CGI)
    size_t qmark = req.uri.find('?');
    if (qmark != std::string::npos)
    {
        req.headers["_query_string"] = req.uri.substr(qmark + 1);
        req.uri = req.uri.substr(0, qmark);
    }

    // Headers
    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size()-1] == '\r')
            line.erase(line.size()-1);
        if (line.empty())
            break;

        size_t pos = line.find(':');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        size_t vs = value.find_first_not_of(" \t");
        if (vs != std::string::npos)
            value = value.substr(vs);

        req.headers[key] = value;
    }

    // Body
    size_t bodyStart = headerEnd + 4;
    if (bodyStart < raw.size())
        req.body = raw.substr(bodyStart);
}

void Server::handleClientRead(size_t i)
{
    int fd = _fds[i].fd;
    char buffer[BUFFER_SIZE];

    int bytes = recv(fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0)
    {
        close(fd);
        _clients.erase(fd);
        _fds.erase(_fds.begin() + i);
        return;
    }

    Client& client = _clients[fd];
    client.appendToBuffer(std::string(buffer, bytes));

    // Enforce client_max_body_size early
    const ServerConfig& config = _serverConfigs[client.getServerIndex()];
    if (config.client_max_body_size > 0 && client.getBuffer().size() > config.client_max_body_size)
    {
        Response res;
        res.statusCode = 413;
        res.body = "413 Content Too Large";
        std::ostringstream ss;
        ss << res.body.size();
        res.headers["Content-Length"] = ss.str();
        res.headers["Content-Type"] = "text/plain";
        res.headers["Connection"] = "close";
        client.getResponse() = res;
        _fds[i].events |= POLLOUT;
        return;
    }

    if (isRequestComplete(client.getBuffer()))
    {
        client.markRequestComplete();
        parseRequest(client);

        Response res = buildResponse(client);
        client.getResponse() = res;

        for (size_t j = 0; j < _fds.size(); ++j)
        {
            if (_fds[j].fd == fd)
            {
                _fds[j].events |= POLLOUT;
                break;
            }
        }
    }
}

void Server::acceptNewClient(int listenSocket) {
    sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    // Accepte la connexion sur le socket spécifique
    int clientFd = accept(listenSocket, (sockaddr*)&clientAddr, &addrLen);
    if (clientFd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return;
    }
    
    // Rend non-bloquant
    fcntl(clientFd, F_SETFL, O_NONBLOCK);
    
    // Trouve l'index du serveur correspondant
    int serverIndex = findServerIndex(listenSocket);
    if (serverIndex == -1) {
        std::cerr << "Error: Could not find server for socket " << listenSocket << std::endl;
        close(clientFd);
        return;
    }
    
    // Crée le client avec son fd et l'index du serveur
    _clients[clientFd] = Client(clientFd, serverIndex);
    
    // Ajoute à poll pour surveillance
    pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);
    
    std::cout << "New client fd=" << clientFd 
              << " on server [" << serverIndex << "] " 
              << _serverConfigs[serverIndex].host << ":"
              << _serverConfigs[serverIndex].port << std::endl;
}

Response Server::buildResponse(Client& client)
{
    Request& req = client.getRequest();
    ServerConfig& config = _serverConfigs[client.getServerIndex()];

    // Validate HTTP method
    if (req.method != "GET" && req.method != "POST" && req.method != "DELETE")
    {
        Response res;
        res.statusCode = 405;
        res.body = "405 Method Not Allowed";
        res.headers["Content-Length"] = intToString((int)res.body.size());
        res.headers["Content-Type"] = "text/plain";
        return res;
    }

    Location* loc = matchLocation(config, req.uri);

    if (!loc)
        return makeErrorResponse(config, 404);

    // Check redirect
    if (!loc->redirect.empty())
    {
        Response res;
        res.statusCode = 301;
        res.headers["Location"] = loc->redirect;
        res.body = "";
        res.headers["Content-Length"] = "0";
        return res;
    }

    // Check allowed methods for this location
    if (!loc->methods.empty())
    {
        bool allowed = false;
        for (size_t i = 0; i < loc->methods.size(); i++)
        {
            if (loc->methods[i] == req.method)
            {
                allowed = true;
                break;
            }
        }
        if (!allowed)
            return makeErrorResponse(config, 405);
    }

    if (req.method == "GET")
        return handleGet(req, config, loc);

    if (req.method == "POST")
        return handlePost(req, config, loc);

    if (req.method == "DELETE")
        return handleDelete(req, config, loc);

    return makeErrorResponse(config, 405);
}

Response Server::makeErrorResponse(const ServerConfig& config, int code)
{
    Response res;
    res.statusCode = code;

    // Try to serve a custom error page
    std::map<int, std::string>::const_iterator it = config.error_pages.find(code);
    if (it != config.error_pages.end())
    {
        std::string root = config.root;
        std::string path = root + it->second;
        std::ifstream file(path.c_str());
        if (file.is_open())
        {
            std::ostringstream ss;
            ss << file.rdbuf();
            res.body = ss.str();
            res.headers["Content-Type"] = getMimeType(path);
            res.headers["Content-Length"] = intToString((int)res.body.size());
            return res;
        }
    }

    // Default error page
    std::ostringstream body;
    body << "<html><head><title>" << code << " " << getStatusText(code) << "</title></head>"
         << "<body><h1>" << code << " " << getStatusText(code) << "</h1></body></html>";
    res.body = body.str();
    res.headers["Content-Type"] = "text/html; charset=UTF-8";
    res.headers["Content-Length"] = intToString((int)res.body.size());
    return res;
}

Location* Server::matchLocation(const ServerConfig& config, const std::string& uri)
{
    Location* best = NULL;
    size_t bestLen = 0;
    for (size_t i = 0; i < config.locations.size(); i++)
    {
        const std::string& locPath = config.locations[i].path;
        if (uri.find(locPath) == 0 && locPath.size() > bestLen)
        {
            bestLen = locPath.size();
            best = (Location*)&config.locations[i];
        }
    }
    return best;
}

bool Server::isCgiRequest(const std::string& path, Location* loc)
{
    if (loc->cgi_extension.empty())
        return false;

    if (path.size() < loc->cgi_extension.size())
        return false;

    return path.substr(path.size() - loc->cgi_extension.size()) == loc->cgi_extension;
}

Response Server::handleGet(const Request& req, const ServerConfig& config, Location* loc)
{
    Response res;

    std::string uri = req.uri;
    const std::string root = !loc->root.empty() ? loc->root : config.root;
    const std::string index = !loc->index.empty() ? loc->index : config.index;

    std::string path = root + uri;

    // Check if it's a directory
    struct stat st;
    bool isDir = (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

    if (isDir)
    {
        // Try index file first
        if (!index.empty())
        {
            std::string indexPath = path;
            if (indexPath.empty() || indexPath[indexPath.size()-1] != '/')
                indexPath += "/";
            indexPath += index;
            std::ifstream indexFile(indexPath.c_str());
            if (indexFile.is_open())
            {
                std::ostringstream buf;
                buf << indexFile.rdbuf();
                res.statusCode = 200;
                res.body = buf.str();
                res.headers["Content-Type"] = getMimeType(indexPath);
                res.headers["Content-Length"] = intToString((int)res.body.size());
                return res;
            }
        }

        // Autoindex
        if (loc->autoindex)
        {
            std::string listing = generateDirectoryListing(path, uri);
            if (!listing.empty())
            {
                res.statusCode = 200;
                res.body = listing;
                res.headers["Content-Type"] = "text/html; charset=UTF-8";
                res.headers["Content-Length"] = intToString((int)res.body.size());
                return res;
            }
        }

        return makeErrorResponse(config, 403);
    }

    if (isCgiRequest(path, loc))
        return executeCgi(req, path, loc, config);

    // Guard: reject directory paths that stat missed (e.g. symlinks to dirs on some OSes)
    if (!path.empty() && path[path.size() - 1] == '/')
        return makeErrorResponse(config, 403);

    std::ifstream file(path.c_str(), std::ios::binary);

    if (!file.is_open())
        return makeErrorResponse(config, 404);

    std::ostringstream buf;
    buf << file.rdbuf();

    res.statusCode = 200;
    res.body = buf.str();
    res.headers["Content-Type"] = getMimeType(path);
    res.headers["Content-Length"] = intToString((int)res.body.size());

    return res;
}

Response Server::handlePost(const Request& req, const ServerConfig& config, Location* loc)
{
    Response res;
    std::string root = !loc->root.empty() ? loc->root : config.root;
    std::string path = root + req.uri;

    if (isCgiRequest(path, loc))
        return executeCgi(req, path, loc, config);

    // File upload: write body to upload_path
    if (!loc->upload_path.empty())
    {
        std::string uploadDir = loc->upload_path;

        // Generate a unique filename using timestamp and PID
        std::ostringstream fname;
        fname << uploadDir << "/upload_" << (unsigned long)time(NULL) << "_" << (unsigned long)getpid();
        std::string uploadFile = fname.str();

        std::ofstream out(uploadFile.c_str(), std::ios::binary);
        if (!out.is_open())
            return makeErrorResponse(config, 500);

        out.write(req.body.c_str(), req.body.size());
        out.close();

        res.statusCode = 201;
        res.body = "File uploaded successfully";
        res.headers["Content-Type"] = "text/plain";
        res.headers["Content-Length"] = intToString((int)res.body.size());
        return res;
    }

    res.statusCode = 200;
    res.body = "POST received";
    res.headers["Content-Type"] = "text/plain";
    res.headers["Content-Length"] = intToString((int)res.body.size());

    return res;
}

Response Server::handleDelete(const Request& req, const ServerConfig& config, Location* loc)
{
    Response res;
    std::string root = !loc->root.empty() ? loc->root : config.root;
    std::string path = root + req.uri;

    if (remove(path.c_str()) == 0)
    {
        res.statusCode = 200;
        res.body = "File deleted";
        res.headers["Content-Type"] = "text/plain";
        res.headers["Content-Length"] = intToString((int)res.body.size());
    }
    else
        return makeErrorResponse(config, 404);

    return res;
}

Response Server::executeCgi(const Request& req, const std::string& scriptPath, Location* loc, const ServerConfig& config)
{
    Response res;

    // Build environment variables for CGI
    std::string queryString = "";
    std::map<std::string, std::string>::const_iterator qit = req.headers.find("_query_string");
    if (qit != req.headers.end())
        queryString = qit->second;

    std::string contentType = "";
    std::map<std::string, std::string>::const_iterator ctit = req.headers.find("Content-Type");
    if (ctit == req.headers.end())
        ctit = req.headers.find("content-type");
    if (ctit != req.headers.end())
        contentType = ctit->second;

    std::string contentLength = intToString((int)req.body.size());

    // Environment array
    std::vector<std::string> envStrs;
    envStrs.push_back("REQUEST_METHOD=" + req.method);
    envStrs.push_back("QUERY_STRING=" + queryString);
    envStrs.push_back("CONTENT_TYPE=" + contentType);
    envStrs.push_back("CONTENT_LENGTH=" + contentLength);
    envStrs.push_back("SCRIPT_FILENAME=" + scriptPath);
    envStrs.push_back("SCRIPT_NAME=" + req.uri);
    envStrs.push_back("PATH_INFO=" + req.uri);
    envStrs.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envStrs.push_back("SERVER_SOFTWARE=webserv/1.0");
    envStrs.push_back("GATEWAY_INTERFACE=CGI/1.1");

    std::vector<char*> envp;
    for (size_t i = 0; i < envStrs.size(); i++)
        envp.push_back(const_cast<char*>(envStrs[i].c_str()));
    envp.push_back(NULL);

    int pipeOut[2];
    int pipeIn[2];
    if (pipe(pipeOut) < 0 || pipe(pipeIn) < 0)
        return makeErrorResponse(config, 500);

    pid_t pid = fork();
    if (pid == 0)
    {
        // Child: redirect stdout to pipeOut, stdin to pipeIn
        dup2(pipeOut[1], STDOUT_FILENO);
        close(pipeOut[0]);
        close(pipeOut[1]);

        dup2(pipeIn[0], STDIN_FILENO);
        close(pipeIn[0]);
        close(pipeIn[1]);

        char* argv[3];
        argv[0] = const_cast<char*>(loc->cgi_path.c_str());
        argv[1] = const_cast<char*>(scriptPath.c_str());
        argv[2] = NULL;

        execve(argv[0], argv, &envp[0]);
        exit(1);
    }

    // Parent: write body to child stdin, read response from stdout
    close(pipeIn[0]);
    if (!req.body.empty())
        write(pipeIn[1], req.body.c_str(), req.body.size());
    close(pipeIn[1]);

    close(pipeOut[1]);

    char buffer[BUFFER_SIZE];
    int bytes;
    std::string output;

    while ((bytes = read(pipeOut[0], buffer, sizeof(buffer))) > 0)
        output.append(buffer, bytes);

    close(pipeOut[0]);
    int childStatus = 0;
    waitpid(pid, &childStatus, 0);

    // If the child produced no output (interpreter not found, script error, etc.), report an error
    if (output.empty())
        return makeErrorResponse(config, 502);

    // Strip CGI headers from output (headers end at first blank line)
    size_t headerEnd = output.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        headerEnd = output.find("\n\n");
    if (headerEnd != std::string::npos)
    {
        std::string cgiHeaders = output.substr(0, headerEnd);
        size_t skip = (output[headerEnd] == '\r') ? 4 : 2;
        res.body = output.substr(headerEnd + skip);

        // Parse Content-Type from CGI headers
        std::istringstream hstream(cgiHeaders);
        std::string hline;
        while (std::getline(hstream, hline))
        {
            if (!hline.empty() && hline[hline.size()-1] == '\r')
                hline.erase(hline.size()-1);
            size_t pos = hline.find(':');
            if (pos != std::string::npos)
            {
                std::string hkey = hline.substr(0, pos);
                std::string hval = hline.substr(pos + 1);
                size_t vs = hval.find_first_not_of(" \t");
                if (vs != std::string::npos)
                    hval = hval.substr(vs);
                res.headers[hkey] = hval;
            }
        }
    }
    else
    {
        res.body = output;
        res.headers["Content-Type"] = "text/html; charset=UTF-8";
    }

    res.statusCode = 200;
    res.headers["Content-Length"] = intToString((int)res.body.size());

    return res;
}