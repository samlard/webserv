#include "../includes/Server.hpp"

Server::Server() {
}

Server::~Server() {

}

void Server::shutdown(){
	//free all and clean server object
}

int Server::init(Config &config) {
	int port = config.getPort();
	std::string host = config.getHost();
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock <= 0)
	{
		std::cout << "error initializing socket" << std::endl;
		return 1;
	}
	// int flags = fcntl(sock, F_GETFL, 0); // sécurité pour pas qu'il écrase des flags
    // fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	fcntl(sock, F_SETFL, O_NONBLOCK);
	struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config.getPort());
    //addr.sin_addr.s_addr = INADDR_ANY;  // ← Ignore host pour simplifier
	if (host == "0.0.0.0")
    	addr.sin_addr.s_addr = INADDR_ANY;
	else
   		addr.sin_addr.s_addr = inet_addr(host.c_str());  // ← IP spécifique
    
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(sock, 128);  // ← SOMAXCONN = 128 souvent
    
    _listenSocket = sock;
    FD_ZERO(&_masterSet);
    FD_SET(sock, &_masterSet);
    _maxFd = sock;
    

	// int opt = 1;
    // setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  faudra mettre pour pas crash quand relance serv avant qu'il nettoie les sockets 
	return 0;
}

void Server::run(){

}