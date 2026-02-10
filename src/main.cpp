#include "Server.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char** argv) {
    int port = 8080;
    
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    
    try {
        Server server(port);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
