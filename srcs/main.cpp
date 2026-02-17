#include "../includes/Server.hpp"
#include "../includes/Config.hpp"


Server* g_server = NULL;

void signalHandler(int signum) {
	(void)signum;
	if (g_server) {
		std::cout << "\nShutting down server..." << std::endl;
		g_server->shutdown();
	}
}

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return 1;
	}
	
	// Parse configuration
	Config config;
	if (!config.parseFile(argv[1])) {
		std::cerr << "Error parsing configuration: " << config.getError() << std::endl;
		return 1;
	}
	
	// // Create and initialize server
	// Server server;
	// g_server = &server;
	
	// if (!server.init(config)) {
	// 	std::cerr << "Failed to initialize server" << std::endl;
	// 	return 1;
	// }
	
	// // Set up signal handlers
	// signal(SIGINT, signalHandler);
	// signal(SIGTERM, signalHandler);
	
	// // Run server
	// std::cout << "Server started successfully" << std::endl;
	// server.run();
	
	// std::cout << "Server stopped" << std::endl;
	return 0;
}
