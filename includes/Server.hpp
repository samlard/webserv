#ifndef SERVER_HPP
# define SERVER_HPP

#include "Config.hpp"
#include <vector> 
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/select.h>
#include <iostream>
#include <map>
#include <string>
#include <poll.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>


class Server {
	public :
		Server();
		~Server();
		void shutdown();
		int init(Config &config);
		void run();

	private :
    	int _listenSocket;        // ← Stocke le socket d'écoute
    	fd_set _masterSet;        // ← Ensemble pour select()
    	int _maxFd;               // ← Plus grand descripteur
    	bool _running;            // ← Pour la boucle run()
		// private:
    	// int _listenSocket;                    // Socket d'écoute principal
    	// std::vector<int> _clientSockets;      // Tous les sockets clients connectés
    	// fd_set _masterSet;                    // Pour select()
    	// int _maxFd;                           // Plus grand fd pour select()
    	// Config _config;                       // Copie de la config (ports, routes...)
    	// bool _running;
};





#endif