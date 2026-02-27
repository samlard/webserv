#include "../includes/Server.hpp"

#define BUFFER_SIZE 4096

Server::Server() {
}

Server::~Server() {

}

void Server::shutdown(){
	//free all and clean server object
    exit(0);
}


int Server::init(Config &config) {
    int port = 8080;
    std::string host;
    (void)config;
    // 1. SOCKET (identique)
    _listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenSocket < 0) {
        perror("socket");
        return 1;
    }

    // 2. NON-BLOQUANT (identique)
    fcntl(_listenSocket, F_SETFL, O_NONBLOCK);
    
    // 3. REUTILISATION PORT (recommandé)
    int opt = 1;
    setsockopt(_listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 4. BIND (identique)
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    host = "0.0.0.0";
    // if (host == "0.0.0.0")
    //     addr.sin_addr.s_addr = INADDR_ANY;
    // else
    //     addr.sin_addr.s_addr = inet_addr(host.c_str());
    
    if (bind(_listenSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(_listenSocket);
        return 1;
    }
    
    // 5. LISTEN (identique)
    if (listen(_listenSocket, 128) < 0) {
        perror("listen");
        close(_listenSocket);
        return 1;
    }
    
    // 6. PREPARER POLL (NOUVEAU)
    _fds.clear();  // ← Vide le vector
    
    pollfd pfd;    // ← Structure pollfd
    pfd.fd = _listenSocket;  // ← Socket à surveiller
    pfd.events = POLLIN;     // ← Surveille lecture (connexions entrantes)
    pfd.revents = 0;         // ← Init à 0
    
    _fds.push_back(pfd);     // ← Ajoute au vector
    
    _running = true;
    
    std::cout << "Listening on " << host << ":" << port << std::endl;
    return 0;
}


void Server::run() {
    // _fds est déjà initialisé dans init() avec _listenSocket
    
    while (_running) {
        // Attente infinie jusqu'à événement (-1 = bloquant jusqu'à activité)
        int ret = poll(_fds.data(), _fds.size(), -1);
        
        if (ret < 0) {
            if (errno == EINTR) continue;  // Signal reçu, on recommande
            perror("poll");
            break;
        }
        
        // Parcourir tous les fds surveillés
        for (size_t i = 0; i < _fds.size(); ++i) {
            int fd = _fds[i].fd;
            short revents = _fds[i].revents;
            
            // === NOUVELLE CONNEXION (socket d'écoute) ===
            if ((revents & POLLIN) && fd == _listenSocket) {
                acceptNewClient();
                continue;
            }
            
            // === DONNÉES CLIENT ===
            if (revents & POLLIN) {
                handleClientRead(i);
            }
            
            // // === PRÊT À ÉCRIRE ===
            // if (revents & POLLOUT) {
            //     handleClientWrite(i);
            // }
            
            // // === ERREUR / DÉCONNEXION ===
            // if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
            //     closeClient(i);
            //     --i;  // Reculer car on a supprimé un élément
            }
        }
  //  }
}

 void Server::handleClientRead(size_t i) {
    int fd = _fds[i].fd;
    char buffer[4096];
    
    int bytes = recv(fd, buffer, 4096, 0);
    
    if (bytes <= 0) {
        // Fermer proprement
        close(fd);
        _fds.erase(_fds.begin() + i);
        return;
    }
    
    // Réponse HTTP
    std::string response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 25\r\n"
        "\r\n"
        "<h1>Hello Webserv!</h1>";
    
    send(fd, response.c_str(), response.length(), 0);
    
    // Fermer la connexion
    //close(fd);
    _fds.erase(_fds.begin() + i);
}


void Server::acceptNewClient() {
    sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    int clientFd = accept(_listenSocket, (sockaddr*)&clientAddr, &addrLen);
    if (clientFd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return;
    }
    
    // Rendre non-bloquant
    fcntl(clientFd, F_SETFL, O_NONBLOCK);
    
    // Ajouter à poll
    pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);
    
    std::cout << "New client: " << clientFd << std::endl;
}
