#include "Server.hpp"
#include <iostream>
#include <signal.h>

static Server* g_server = NULL;

void signal_handler(int signum) {
	(void)signum;
	if (g_server) {
		std::cout << "\nShutting down server..." << std::endl;
		g_server->stop();
	}
}

int main(int argc, char** argv) {
	std::string config_file = "config/default.conf";
	
	if (argc > 1)
		config_file = argv[1];
	
	Server server(config_file);
	g_server = &server;
	
	// Setup signal handlers
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	
	if (!server.initialize()) {
		std::cerr << "Error: Failed to initialize server" << std::endl;
		return 1;
	}
	
	server.run();
	
	return 0;
}
