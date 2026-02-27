#include "../includes/Config.hpp"

Config::Config(){
}

Config::~Config() {
}

std::string Config::getError() const {
    return _errorMsg + "\n";
}

bool Config::is_valid_server_directive(const std::string& key) {
    return key == "listen" || key == "host" || key == "server_name" ||
           key == "root" || key == "index" || key == "error_page" ||
           key == "client_max_body_size";
}

bool Config::is_valid_location_directive(const std::string& key) {
    return key == "root" || key == "index" || key == "allow_methods" ||
           key == "methods" || key == "autoindex" || key == "upload_path" ||
           key == "cgi_extension" || key == "cgi_path" || key == "return";
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

int Config::fill_location(std::istringstream &iss, Location &loc, std::string &error)
{

}

int Config::fill_server(std::string &ServerBlock, std::string &error)
{
    ServerConfig serv;
    std::istringstream iss(ServerBlock);
    std::string line;
    
    std::getline(iss, line);
    while (std::getline(iss, line)) {
        std::string trimmed = trim(line);
        if (trimmed == "}" || trimmed.find("}") != std::string::npos)
            break;

        if (trimmed.empty() || trimmed[0] == '#')
            continue;
        
        if (trimmed.find("location") == 0) {
            Location loc;
            if (fill_location(iss, loc, error) != 0) {
                return 1;
            }
            serv.locations.push_back(loc);
            continue;
        }
        
        // // Parse directive simple
        // std::string key, value;
        // if (!parse_directive(trimmed, key, value)) {
        //     error = "Invalid syntax: '" + trimmed + "'";
        //     return 1;
        // }
        
        // // Vérification directive valide
        // if (!is_valid_server_directive(key)) {
        //     error = "Unknown directive '" + key + "'";
        //     return 1;
        // }
        
        // Remplissage
        if (key == "listen") {
            // Vérification nombre
            for (size_t i = 0; i < value.length(); i++) {
                if (!isdigit(value[i])) {
                    error = "Invalid port: '" + value + "'";
                    return 1;
                }
            }
            serv.port = atoi(value.c_str());
        }
        else if (key == "host") {
            serv.host = value;
        }
        else if (key == "server_name") {
            serv.server_names.push_back(value);
        }
        else if (key == "root") {
            serv.root = value;
        }
        else if (key == "index") {
            serv.index = value;
        }
        // ... autres directives
    }
    
    // Validation minimale
    if (serv.port == 0) {
        error = "Missing 'listen' directive";
        return 1;
    } 
    _servers.push_back(serv);
    return 0;  // Succès
}

int Config::parseFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        _errorMsg = "Cannot open file: " + filename;
        return 1;
    }
    std::string line;
    std::string currentBlock;
    bool inServer = false;
    int braceCount = 0;
    
    while (std::getline(file, line)) {
        if (!inServer) {
            std::string trimmed = trim(line);
            if (trimmed.find("server") == 0 && trimmed.find("{") != std::string::npos) {
                inServer = true;
                braceCount = 1;
                currentBlock = line + "\n";
            }
            continue;
        }
        currentBlock += line + "\n";

        for (size_t i = 0; i < line.length(); i++) {
            if (line[i] == '{') braceCount++;
            else if (line[i] == '}') braceCount--;
        }

        if (braceCount == 0) {
            std::string error;
            std::cout << currentBlock << std::endl;
            if (fill_server(currentBlock, error) == 0) {
                std::cout << "Server added\n";
            } else {
                std::cout << "Server skipped: " << error << "\n";
            }
            inServer = false;
            currentBlock.clear();
        }
    }
    
    if (braceCount != 0) {
        _errorMsg = "Problem with brackets";
        return 1;
    }

    if (_servers.empty()) {
        _errorMsg = "No valid server found";
        return 1;
    }

    return 0;
}