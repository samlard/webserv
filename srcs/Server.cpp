#include "../includes/Server.hpp"

#define BUFFER_SIZE 4096

Server::Server() {
}

Server::~Server() {

}

void Server::shutdown(){
	//free all and clean server object
}

// int Server::init(Config &config) {
// 	int port = config.getPort();
// 	std::string host = config.getHost();
// 	int sock = socket(AF_INET, SOCK_STREAM, 0);
// 	if (sock <= 0)
// 	{
// 		std::cout << "error initializing socket" << std::endl;
// 		return 1;
// 	}
// 	// int flags = fcntl(sock, F_GETFL, 0); // sécurité pour pas qu'il écrase des flags
//     // fcntl(sock, F_SETFL, flags | O_NONBLOCK);
// 	fcntl(sock, F_SETFL, O_NONBLOCK);
// 	struct sockaddr_in addr;
//     memset(&addr, 0, sizeof(addr));
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(config.getPort());
//     //addr.sin_addr.s_addr = INADDR_ANY;  // ← Ignore host pour simplifier
// 	if (host == "0.0.0.0")
//     	addr.sin_addr.s_addr = INADDR_ANY;
// 	else
//    		addr.sin_addr.s_addr = inet_addr(host.c_str());  // ← IP spécifique
    
//     bind(sock, (struct sockaddr*)&addr, sizeof(addr));
//     listen(sock, 128);  // ← SOMAXCONN = 128 souvent
    
//     _listenSocket = sock;
//     FD_ZERO(&_masterSet);
//     FD_SET(sock, &_masterSet);
//     _maxFd = sock;
    

// 	// int opt = 1;
//     // setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  faudra mettre pour pas crash quand relance serv avant qu'il nettoie les sockets 
// 	return 0;
// } // si utilisation de select mais poll plus reccomandé
int Server::init(Config &config) {
    int port = config.getPort();
    std::string host = config.getHost();
    
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
    
    if (host == "0.0.0.0")
        addr.sin_addr.s_addr = INADDR_ANY;
    else
        addr.sin_addr.s_addr = inet_addr(host.c_str());
    
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
        
        // // Parcourir tous les fds surveillés
        // for (size_t i = 0; i < _fds.size(); ++i) {
        //     int fd = _fds[i].fd;
        //     short revents = _fds[i].revents;
            
            // // === NOUVELLE CONNEXION (socket d'écoute) ===
            // if ((revents & POLLIN) && fd == _listenSocket) {
            //     acceptNewClient();
            //     continue;
            // }
            
            // // === DONNÉES CLIENT ===
            // if (revents & POLLIN) {
            //     handleClientRead(i);
            // }
            
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
//}

// void Server::run(){

//     std::vector<pollfd> fds;
//     std::map<int, Client> clients;

//     // fds.push_back({server1, POLLIN, 0});
//     // fds.push_back({server2, POLLIN, 0});

//     while (true) {

//         // nettoyage zombies CGI
//         while (waitpid(-1, NULL, WNOHANG) > 0);

//         poll(fds.data(), fds.size(), -1);

//         for (size_t i = 0; i < fds.size(); ++i) {

//             int fd = fds[i].fd;

//             // === NOUVELLE CONNEXION ===
//             if (fds[i].revents & POLLIN) {

//                 if (fd == server1 || fd == server2) {

//                     int client_fd = accept(fd, NULL, NULL);
//                     if (client_fd < 0) continue;

//                     make_nonblocking(client_fd);

//                     fds.push_back({client_fd, POLLIN, 0});
//                     clients.insert(std::make_pair(
//                         client_fd,
//                         Client(client_fd, fd)
//                     ));

//                 } else {
//                     // === CLIENT EXISTANT ENVOIE DATA ===

//                     char buffer[BUFFER_SIZE];
//                     int bytes = recv(fd, buffer, BUFFER_SIZE, 0);

//                     if (bytes <= 0) {
//                         close(fd);
//                         fds.erase(fds.begin() + i);
//                         clients.erase(fd);
//                         --i;
//                         continue;
//                     }

//                     Client &c = clients[fd];
//                     c.request.append(buffer, bytes);

//                     // vérifier fin headers
//                     size_t pos;
//                     if (!c.headers_parsed &&
//                         (pos = c.request.find("\r\n\r\n")) != std::string::npos) {

//                         c.headers_parsed = true;

//                         std::string headers = c.request.substr(0, pos + 4);

//                         // parser Content-Length si besoin
//                         size_t cl = headers.find("Content-Length:");
//                         if (cl != std::string::npos) {
//                             size_t end = headers.find("\r\n", cl);
//                             c.expected_body = std::stoi(
//                                 headers.substr(cl + 15, end - (cl + 15))
//                             );
//                         }

//                         // si pas de body → prêt à traiter
//                         if (c.expected_body == 0) {

//                             // === ICI TU FERAS FORK + EXEC ===
//                             // fork();
//                             // execve();
//                             // récupérer sortie CGI via pipe

//                             c.response =
//                                 "HTTP/1.1 200 OK\r\n"
//                                 "Content-Length: 5\r\n\r\nHello";

//                             fds[i].events |= POLLOUT;
//                         }
//                     }

//                     // si body attendu
//                     if (c.headers_parsed && c.expected_body > 0) {

//                         size_t body_start =
//                             c.request.find("\r\n\r\n") + 4;

//                         if (c.request.size() - body_start
//                             >= c.expected_body) {

//                             std::string body =
//                                 c.request.substr(body_start,
//                                                  c.expected_body);

//                             // === ICI TU FERAS FORK + EXEC ===
//                             // fork();
//                             // execve();
//                             // passer body à CGI via pipe

//                             c.response =
//                                 "HTTP/1.1 200 OK\r\n"
//                                 "Content-Length: 2\r\n\r\nOK";

//                             fds[i].events |= POLLOUT;
//                         }
//                     }
//                 }
//             }

//             // === ENVOI REPONSE ===
//             if (fds[i].revents & POLLOUT) {

//                 Client &c = clients[fd];

//                 int sent = send(fd,
//                                 c.response.c_str(),
//                                 c.response.size(),
//                                 0);

//                 if (sent > 0)
//                     c.response.erase(0, sent);

//                 if (c.response.empty()) {
//                     fds[i].events &= ~POLLOUT;
//                 }
//             }

//             // === ERREUR / HANGUP ===
//             if (fds[i].revents & (POLLHUP | POLLERR)) {
//                 close(fd);
//                 fds.erase(fds.begin() + i);
//                 clients.erase(fd);
//                 --i;
//             }
//         }
//     }
// }