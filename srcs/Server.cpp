#include "../includes/Server.hpp"

#define BUFFER_SIZE 4096

Server::Server() {
}

Server::~Server() {

}

void Server::shutdown(){
    exit(0);
}

static bool isRequestComplete(const std::string& buffer)
{
    return buffer.find("\r\n\r\n") != std::string::npos;
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

void Server::run() {
    while (_running) {

        int ret = poll(_fds.data(), _fds.size(), -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        for (int i = (int)_fds.size() - 1; i >= 0; i--) {
            int fd = _fds[i].fd;
            short revents = _fds[i].revents;

            if (revents == 0) continue;

            bool isListenSocket = false;
            for (size_t j = 0; j < _listenSockets.size(); j++) {
                if (_listenSockets[j] == fd) {
                    isListenSocket = true;
                    break;
                }
            }

            // Nouvelle connexion
            if (isListenSocket && (revents & POLLIN)) {
                acceptNewClient(fd);
                continue;
            }

            // Données reçues
            if (!isListenSocket && (revents & POLLIN)) {
                handleClientRead(i);
            }

            // Envoi de la réponse
            if (!isListenSocket && (_fds[i].revents & POLLOUT)) {
                Client &client = _clients[fd];
                std::string responseStr = client.getResponse().toString(); // copie locale

                ssize_t sent = send(fd, responseStr.c_str(), responseStr.size(), 0);

                if (sent < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        close(fd);
                        _clients.erase(fd);
                        _fds.erase(_fds.begin() + i);
                    }
                    continue;
                }

                // Supprime ce qui a été envoyé
                client.getResponse().body.erase(0, sent);

                // Si tout a été envoyé, on arrête POLLOUT
                if (client.getResponse().body.empty()) {
                    _fds[i].events &= ~POLLOUT;
                    close(fd);
                    _clients.erase(fd);
                    _fds.erase(_fds.begin() + i);
                    continue;
                }
            }

            // Gestion des erreurs
            if (_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                close(fd);
                _clients.erase(fd);
                _fds.erase(_fds.begin() + i);
            }
        }
    }
}

// void Server::run() {
//     while (_running) {

//         int ret = poll(_fds.data(), _fds.size(), -1);
//         if (ret < 0) {
//             if (errno == EINTR) continue;
//             perror("poll");
//             break;
//         }
        
//         for (int i = (int)_fds.size() - 1; i >= 0; i--) {
//             int fd = _fds[i].fd;
//             short revents = _fds[i].revents;
            
//             if (revents == 0) continue;  // Pas d'activité
            
//             bool isListenSocket = false;
//             for (size_t j = 0; j < _listenSockets.size(); j++) {
//                 if (_listenSockets[j] == fd) {
//                     isListenSocket = true;
//                     break;
//                 }
//             }
//             if (isListenSocket && (revents & POLLIN)) {
//                 acceptNewClient(fd);
//                 continue;
//             }
        
//             if (!isListenSocket && (revents & POLLIN)) 
//                 handleClientRead(i);
//             if (_fds[i].revents & POLLOUT) {
//                 Client &client = _clients[fd];
//                 std::string& responseStr = client.getResponse().toString(); 
//                 int sent = send(fd, responseStr.c_str(), responseStr.size(), 0);
//                 responseStr.erase(0, sent);
//             }
//             if (responseStr.empty()) {
//                 _fds[i].events &= ~POLLOUT; // plus besoin d'écrire
//                 close(fd); // fermer le socket ou réinitialiser le client
//                 _clients.erase(fd);
//                 _fds.erase(_fds.begin() + i);
//     }
//             if (client.getResponse().body.empty()) {
//                 _fds[i].events &= ~POLLOUT; // plus besoin d'écrire
//             }
//             if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
//                 close(fd);
//                 _clients.erase(fd);
//                 _fds.erase(_fds.begin() + i);
//                 // Pas de i-- car parcours inverse
//             }
//         }
//     }
// }


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
}

void Server::handleClientRead(size_t i)
{
    int fd = _fds[i].fd;

    char buffer[4096];

    int bytes = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes <= 0)
        return;

    Client& client = _clients[fd];

    client.appendToBuffer(std::string(buffer, bytes));

    if (isRequestComplete(client.getBuffer()))
    {
        client.markRequestComplete();

        parseRequest(client);
        Response res = buildResponse(client);

        client.getResponse() = res;

        for (size_t j = 0; j < _fds.size(); j++)
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

    Location* loc = matchLocation(config, req.uri);

    if (!loc)
    {
        Response res;
        res.statusCode = 404;
        res.body = "404 Not Found";
        return res;
    }

    if (req.method == "GET")
        return handleGet(req, config, loc);

    if (req.method == "POST")
        return handlePost(req, config, loc);

    if (req.method == "DELETE")
        return handleDelete(req, config, loc);

    Response res;
    res.statusCode = 405;
    res.body = "Method Not Allowed";
    return res;
}

Location* Server::matchLocation(const ServerConfig& config, const std::string& uri)
{
    for (size_t i = 0; i < config.locations.size(); i++)
    {
        if (uri.find(config.locations[i].path) == 0)
            return (Location*)&config.locations[i];
    }
    return NULL;
}

bool Server::isCgiRequest(const std::string& path, Location* loc)
{
    if (loc->cgi_extension.empty())
        return false;

    if (path.size() < loc->cgi_extension.size())
        return false;

    return path.substr(path.size() - loc->cgi_extension.size()) == loc->cgi_extension;
}

Response Server::handleGet(const Request& req, const ServerConfig& /*config*/, Location* loc)
{
    Response res;

   std::string uri = req.uri;

   std::cout << "Requested URI: " << uri << std::endl;

    // Si c'est "/", redirige vers index.html
    if (uri == "/" && !loc->index.empty()) {
        uri = "/" + loc->index;
    }


    std::string path = loc->root + uri;

    std::cout << "Requested I: " << uri << std::endl;

    if (isCgiRequest(path, loc))
        return executeCgi(req, path, loc);

    std::ifstream file(path.c_str());

    if (!file.is_open())
    {
        res.statusCode = 404;
        res.body = "404 Not Found";
        return res;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    res.statusCode = 200;
    res.body = buffer.str();

    // C++98 workaround pour std::to_string
    std::stringstream ss;
    ss << res.body.size();
    res.headers["Content-Length"] = ss.str();
    res.headers["Content-Type"] = "text/html";

    return res;
}

Response Server::handlePost(const Request& req, const ServerConfig& config, Location* loc)
{
    (void)config;

    Response res;
    std::string path = loc->root + req.uri;

    if (isCgiRequest(path, loc))
        return executeCgi(req, path, loc);

    res.statusCode = 200;
    res.body = "POST received";

    return res;
}

Response Server::handleDelete(const Request& req, const ServerConfig& config, Location* loc)
{
    (void)config;

    Response res;
    std::string path = loc->root + req.uri;

    if (remove(path.c_str()) == 0)
    {
        res.statusCode = 200;
        res.body = "File deleted";
    }
    else
    {
        res.statusCode = 404;
        res.body = "File not found";
    }

    return res;
}

Response Server::executeCgi(const Request& req, const std::string& scriptPath, Location* loc)
{
    (void)req; // supprime warning unused-parameter

    Response res;
    int pipefd[2];
    pipe(pipefd);

    pid_t pid = fork();
    if (pid == 0)
    {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);

        char* argv[3];
        argv[0] = const_cast<char*>(loc->cgi_path.c_str());
        argv[1] = const_cast<char*>(scriptPath.c_str());
        argv[2] = NULL;

        execve(argv[0], argv, NULL);
        exit(1);
    }

    close(pipefd[1]);

    char buffer[4096];
    int bytes;
    std::string output;

    while ((bytes = read(pipefd[0], buffer, 4096)) > 0)
        output.append(buffer, bytes);

    close(pipefd[0]);
    waitpid(pid, NULL, 0);

    res.statusCode = 200;
    res.body = output;

    std::stringstream ss;
    ss << res.body.size();
    res.headers["Content-Length"] = ss.str();
    res.headers["Content-Type"] = "text/html";

    return res;
}