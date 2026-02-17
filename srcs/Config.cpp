#include "../includes/Config.hpp"

Config::Config() : _port(8080), _host("0.0.0.0"), _root("./www"), _index("index.html") {
}

Config::~Config() {
}

int Config::parseFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cout << "Cannot open file: " << filename << std::endl;
        return 1;
    }

    std::string line;
    
    while (std::getline(file, line)) {
        // Enlever espaces début/fin
        while (!line.empty() && (line[0] == ' ' || line[0] == '\t'))
            line = line.substr(1);
        while (!line.empty() && (line[line.length()-1] == ' ' || line[line.length()-1] == '\t'))
            line = line.substr(0, line.length()-1);
        
        // Ignorer vide ou commentaire
        if (line.empty() || line[0] == '#') 
            continue;
        
        // Trouver espace
        size_t space = line.find(' ');
        if (space == std::string::npos) 
            continue;
        
        std::string key = line.substr(0, space);
        std::string value = line.substr(space + 1);
        
        // Nettoyer value
        while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
            value = value.substr(1);
        while (!value.empty() && value[value.length()-1] == ';')
            value = value.substr(0, value.length()-1);
        
        // Remplir directement les variables membres
        if (key == "listen") {
            _port = atoi(value.c_str());
        }
        else if (key == "host") {
            _host = value;
        }
        else if (key == "root") {
            _root = value;
        }
        else if (key == "index") {
            _index = value;
        }
    }
    return 1;
}

std::string Config::getError() const {
    return "error\n";
}

// Getters
int Config::getPort() const { return _port; }
std::string Config::getHost() const { return _host; }
std::string Config::getRoot() const { return _root; }
std::string Config::getIndex() const { return _index; }