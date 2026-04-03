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
    loc.autoindex = false;

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
        
        if (trimmed == "}" || trimmed.find("}") != std::string::npos)
            break;

        if (trimmed.empty() || trimmed[0] == '#')
            continue;
        
        if (trimmed[trimmed.length() - 1] != ';') {
            error = "Missing semicolon in location " + loc.path + ": '" + trimmed + "'";
            return 1;
        }
        
        std::string noSemi = trimmed.substr(0, trimmed.length() - 1);
        
        std::istringstream lineIss(noSemi);
        std::string key;
        lineIss >> key;

        if (!is_valid_location_directive(key)) {
            error = "Unknown directive '" + key + "' in location " + loc.path;
            return 1;
        }
    
        std::string value;
        std::getline(lineIss, value);
        value = trim(value);
        
        if (key == "root") {
            if (value.empty()) {
                error = "root requires a path in location " + loc.path;
                return 1;
            }
            loc.root = value;
        }
        else if (key == "index") {
            if (value.empty()) {
                error = "index requires a filename in location " + loc.path;
                return 1;
            }
            loc.index = value;
        }   
        else if (key == "allow_methods" || key == "methods") 
        {
            if (value.empty()) {
                error = "allow_methods requires at least one method in location " + loc.path;
                return 1;
            }
            std::istringstream m(value);
            std::string method;
            while (m >> method) {
                if (method != "GET" && method != "POST" && method != "DELETE" && method != "HEAD") {
                    error = "Invalid method '" + method + "' in location " + loc.path;
                    return 1;
                }
                loc.methods.push_back(method);
            }
        }
        
        else if (key == "autoindex") 
        {
            if (value != "on" && value != "off") {
                error = "autoindex must be 'on' or 'off' in location " + loc.path + ", found: " + value;
                return 1;
            }
            loc.autoindex = (value == "on");
        }
        else if (key == "cgi_extension") 
        {
            std::istringstream extStream(value);
            std::string ext;
            while (extStream >> ext) 
            {
                if (ext.empty())
                    continue;
                if (ext[0] != '.')
                {
                    error = "cgi_extension must start with '.', found: " + ext + " in location " + loc.path;
                    return 1;
                }
                loc.cgi_extensions.push_back(ext);
            }
            if (loc.cgi_extensions.empty()) 
            {
                error = "cgi_extension requires at least one extension in location " + loc.path;
                return 1;
            }
        }
        
        else if (key == "cgi_path") 
        {
            if (value.empty()) {
                error = "cgi_path requires a path in location " + loc.path;
                return 1;
            }
            loc.cgi_path = value;
        }
        else if (key == "upload_path") 
        {
            if (value.empty()) {
                error = "upload_path requires a path in location " + loc.path;
                return 1;
            }
            loc.upload_path = value;
        }
        else if (key == "return") 
        {
            if (value.empty()) 
            {
                error = "return requires a URL or status code + URL in location " + loc.path;
                return 1;
            }
            loc.redirect = value;
        }
    }
    
    return 0;
}

int Config::fill_server(std::string &ServerBlock, std::string &error)
{
    ServerConfig serv;
    serv.port = 0;
    serv.host = "0.0.0.0";
    serv.client_max_body_size = 1000000;
    _listen = false;

    std::istringstream iss(ServerBlock);
    std::string line;

    std::getline(iss, line);

    while (std::getline(iss, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#')
            continue;

        if (trimmed == "}" || trimmed.find("}") != std::string::npos)
            break;

        if (trimmed.find("location") == 0) 
        {
            Location loc;
            std::string locBlock = trimmed + "\n";
            std::string locLine;
            int braceCount = 0;

            for (size_t i = 0; i < trimmed.size(); i++)
                if (trimmed[i] == '{') braceCount++;

            while (braceCount > 0 && std::getline(iss, locLine)) 
            {
                locBlock += locLine + "\n";
                for (size_t i = 0; i < locLine.size(); i++) 
                {
                    if (locLine[i] == '{') 
                        braceCount++;
                    else if (locLine[i] == '}') 
                        braceCount--;
                }
            }
            std::istringstream locStream(locBlock);
            if (fill_location(locStream, loc, error) != 0)
                return 1;
            serv.locations.push_back(loc);
            continue;
        }

        if (trimmed[trimmed.length() - 1] != ';') 
        {
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

        if (!is_valid_server_directive(key)) 
        {
            error = "Unknown directive '" + key + "' in server block";
            return 1;
        }

        if (key == "listen") 
        {
            if (_listen == true)
            {
                error = "cant have multiple listen in same serv";
                return 1;
            }
            _listen = true;
            if (value.empty()) 
            {
                error = "listen requires a port number";
                return 1;
            }
            for (size_t i = 0; i < value.length(); i++) 
            {
                if (!isdigit(value[i])) 
                {
                    error = "listen must be a number, found: '" + value + "'";
                    return 1;
                }
            }
            int port = atoi(value.c_str());
            if (port < 1 || port > 65535) 
            {
                error = "listen port out of range (1-65535): '" + value + "'";
                return 1;
            }
            serv.port = port;
        }
        else if (key == "host")
            serv.host = value;
        else if (key == "server_name")
            serv.server_names.push_back(value);
        else if (key == "root")
            serv.root = value;
        else if (key == "index")
            serv.index = value;
        else if (key == "client_max_body_size") 
        {
            if (value.empty()) {
                error = "client_max_body_size requires a numeric value";
                return 1;
            }
            for (size_t i = 0; i < value.length(); i++) 
            {
                if (!isdigit(value[i])) 
                {
                    error = "client_max_body_size must be a number, found: '" + value + "'";
                    return 1;
                }
            }
            serv.client_max_body_size = static_cast<size_t>(atoi(value.c_str()));
        } 
        else if (key == "error_page") 
        {
            std::istringstream epIss(value);
            int code;
            std::string page;
            if (epIss >> code >> page) 
                serv.error_pages[code] = page;
            else 
            {
                error = "error_page requires a status code and a path";
                return 1;
            }
        }
    }
    if (serv.port == 0) 
    {
        error = "Missing 'listen' directive in server block";
        return 1;
    }
    _servers.push_back(serv);
    return 0;
}

int Config::parseFile(const std::string& filename) 
{
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        _errorMsg = "Cannot open file: " + filename;
        return 1;
    }
    std::string line;
    std::string currentBlock;
    bool inServer = false;
    int braceCount = 0;
    while (std::getline(file, line))
    {
        if (!inServer) 
        {
            std::string trimmed = trim(line);
            if (trimmed.find("server") == 0 && trimmed.find("{") != std::string::npos) 
            {
                inServer = true;
                braceCount = 1;
                currentBlock = line + "\n";
            }
            continue;
        }
        currentBlock += line + "\n";
        for (size_t i = 0; i < line.length(); i++) 
        {
            if (line[i] == '{') braceCount++;
            else if (line[i] == '}') braceCount--;
        }

        if (braceCount == 0) 
        {
            std::string error;
            if (fill_server(currentBlock, error) == 0)
                std::cout << "Server added\n";
            else
                std::cout << "Server skipped: " << error << "\n";
            inServer = false;
            currentBlock.clear();
        }
    }
    if (braceCount != 0) 
    {
        _errorMsg = "Problem with brackets";
        return 1;
    }
    if (_servers.empty()) 
    {
        _errorMsg = "No valid server found";
        return 1;
    }
    for (size_t i = 0; i < _servers.size(); i++) 
    {
        for (size_t j = i + 1; j < _servers.size(); j++) 
        {
            if (_servers[i].port == _servers[j].port && _servers[i].host == _servers[j].host) 
            {
                std::stringstream ss;
                ss << "Duplicate server on " << _servers[i].host << ":" << _servers[i].port;
                _errorMsg = ss.str();
                return 1;
            }
        }
    }
    return 0;
}

const std::vector<ServerConfig>& Config::getServers() const
{
    return _servers;
}