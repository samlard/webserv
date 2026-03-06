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
           key == "cgi_extension" || key == "cgi_path";
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
    std::string firstLine;
    std::getline(iss, firstLine);
    
    std::istringstream firstIss(trim(firstLine));
    std::string locKeyword, path, brace;
    
    firstIss >> locKeyword >> path >> brace;
    if (locKeyword != "location") {
        error = "Expected 'location', found: " + locKeyword;
        return 1;
    }
    if (path.empty()) {
        error = "Missing path in location";
        return 1;
    }
    loc.path = path;
    
    if (brace != "{") {
        error = "Expected '{', found: " + (brace.empty() ? "nothing" : brace);
        return 1;
    }
    
    std::string line;
    while (std::getline(iss, line)) {
        std::string trimmed = trim(line);
        
        // Fin du bloc
        if (trimmed == "}" || trimmed.find("}") != std::string::npos)
            break;
        
        // Skip vide et commentaires
        if (trimmed.empty() || trimmed[0] == '#')
            continue;
        
        // Vérifie le ; final
        if (trimmed[trimmed.length() - 1] != ';') {
            error = "Missing semicolon in location " + loc.path + ": '" + trimmed + "'";
            return 1;
        }
        
        // Enlève le ;
        std::string noSemi = trimmed.substr(0, trimmed.length() - 1);
        
        // Sépare key et value
        std::istringstream lineIss(noSemi);
        std::string key;
        lineIss >> key;
        
        // Vérifie directive valide
        if (!is_valid_location_directive(key)) {
            error = "Unknown directive '" + key + "' in location " + loc.path;
            return 1;
        }
        
        // Récupère la valeur
        std::string value;
        std::getline(lineIss, value);
        value = trim(value);
        
        // ========== REMPLISSAGE OBLIGATOIRE 42 ==========
        
        // root : path
        if (key == "root") {
            if (value.empty()) {
                error = "root requires a path in location " + loc.path;
                return 1;
            }
            loc.root = value;
        }
        
        // index : fichier
        else if (key == "index") {
            if (value.empty()) {
                error = "index requires a filename in location " + loc.path;
                return 1;
            }
            loc.index = value;
        }
        
        // allow_methods : GET POST DELETE uniquement
        else if (key == "allow_methods" || key == "methods") {
            if (value.empty()) {
                error = "allow_methods requires at least one method in location " + loc.path;
                return 1;
            }
            std::istringstream m(value);
            std::string method;
            while (m >> method) {
                if (method != "GET" && method != "POST" && method != "DELETE") {
                    error = "Invalid method '" + method + "' in location " + loc.path;
                    return 1;
                }
                loc.methods.push_back(method);
            }
        }
        
        // autoindex : on ou off uniquement
        else if (key == "autoindex") {
            if (value != "on" && value != "off") {
                error = "autoindex must be 'on' or 'off' in location " + loc.path + ", found: " + value;
                return 1;
            }
            // loc.autoindex = (value == "on");
        }
        
        // cgi_extension : doit commencer par .
        else if (key == "cgi_extension") {
            if (value.empty()) {
                error = "cgi_extension requires an extension in location " + loc.path;
                return 1;
            }
            if (value[0] != '.') {
                error = "cgi_extension must start with '.', found: " + value + " in location " + loc.path;
                return 1;
            }
            loc.cgi_extension = value;
        }
        
        // cgi_path : path exécutable
        else if (key == "cgi_path") {
            if (value.empty()) {
                error = "cgi_path requires a path in location " + loc.path;
                return 1;
            }
            loc.cgi_path = value;
        }
    }
    
    return 0;
}

int Config::fill_server(std::string &ServerBlock, std::string &error)
{
    ServerConfig serv;
    std::istringstream iss(ServerBlock);
    std::string line;

    // Skip la première ligne "server {"
    std::getline(iss, line);

    while (std::getline(iss, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#')
            continue;

        // Fin du bloc serveur
        if (trimmed == "}" || trimmed.find("}") != std::string::npos)
            break;

        // ======= Gestion des locations =======
        if (trimmed.find("location") == 0) {
            Location loc;

            // Construit tout le bloc location
            std::string locBlock = trimmed + "\n";
            std::string locLine;
            int braceCount = 0;

            // Compte les { de la ligne de départ
            for (size_t i = 0; i < trimmed.size(); i++)
                if (trimmed[i] == '{') braceCount++;

            // Lit toutes les lignes jusqu'à équilibrage des accolades
            while (braceCount > 0 && std::getline(iss, locLine)) {
                locBlock += locLine + "\n";
                for (size_t i = 0; i < locLine.size(); i++) {
                    if (locLine[i] == '{') braceCount++;
                    else if (locLine[i] == '}') braceCount--;
                }
            }

            std::istringstream locStream(locBlock);
            if (fill_location(locStream, loc, error) != 0) {
                return 1;
            }

            serv.locations.push_back(loc);
            continue;
        }

        // ======= Directives serveur =======
        if (trimmed[trimmed.length() - 1] != ';') {
            error = "Missing semicolon: '" + trimmed + "'";
            return 1;
        }

        std::string noSemi = trimmed.substr(0, trimmed.length() - 1);
        std::istringstream lineIss(noSemi);
        std::string key;
        lineIss >> key;

        std::string value;
        std::getline(lineIss, value);
        value = trim(value);

        if (!is_valid_server_directive(key)) {
            error = "Unknown directive '" + key + "' in server block";
            return 1;
        }

        if (key == "listen") {
            if (value.empty()) {
                error = "listen requires a port number";
                return 1;
            }
            for (size_t i = 0; i < value.length(); i++) {
                if (!isdigit(value[i])) {
                    error = "listen must be a number, found: '" + value + "'";
                    return 1;
                }
            }
            int port = atoi(value.c_str());
            if (port < 1 || port > 65535) {
                error = "listen port out of range (1-65535): '" + value + "'";
                return 1;
            }
            serv.port = port;
        } else if (key == "host") {
            serv.host = value;
        } else if (key == "server_name") {
            serv.server_names.push_back(value);
        } else if (key == "root") {
            serv.root = value;
        } else if (key == "index") {
            serv.index = value;
        }
        // autres directives optionnelles : error_page, client_max_body_size
    }

    // Validation finale
    if (serv.port == 0) {
        error = "Missing 'listen' directive in server block";
        return 1;
    }

    _servers.push_back(serv);
    return 0;
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

const std::vector<ServerConfig>& Config::getServers() const
{
    return _servers;
}