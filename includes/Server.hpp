#ifndef SERVER_HPP
# define SERVER_HPP

#include "Config.hpp"
#include <vector> 

class Server {
	public :
		Server();
		~Server();
		void shutdown();
		int init(Config &config);
		void run();

	private :
		// private:
    	// int _listenSocket;                    // Socket d'écoute principal
    	// std::vector<int> _clientSockets;      // Tous les sockets clients connectés
    	// fd_set _masterSet;                    // Pour select()
    	// int _maxFd;                           // Plus grand fd pour select()
    	// Config _config;                       // Copie de la config (ports, routes...)
    	// bool _running;
};





#endif