#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <string.h>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>


class Location {
	public:
    	std::string                 path;
    	std::string                 root;
    	std::string                 index;
    	std::vector<std::string>       methods;
    	bool                        autoindex;
   		std::string                 redirect;
    	std::vector<std::string>    cgi_extensions;
    	std::string                 cgi_path;
    	std::string                 upload_path;
    
    Location(){};
    ~Location(){};
};

class ServerConfig {
	public:
    	int                         port;
    	std::string                 host;
    	std::vector<std::string>    server_names;
    	std::string                 root;
    	std::string                 index;
    	std::map<int, std::string>  error_pages;
    	size_t                      client_max_body_size;
    	std::vector<Location>       locations;
    
    	ServerConfig(){};
    	~ServerConfig(){};
};



class Config {
	private :
   	 	std::vector<ServerConfig> _servers;
		std::string _errorMsg;       


	public :
		Config();
		~Config();
		std::string get_server_block(std::istringstream &iss, std::string& erroblock);
		int fill_server(std::string &ServerBlock, std::string &error);
		int parseFile(const std::string& filename);
		const std::vector<ServerConfig>& getServers() const;
		std::string getError() const;
		int fill_location(std::istringstream &iss, Location &loc, std::string &error);
		bool is_valid_server_directive(const std::string& key);
		bool is_valid_location_directive(const std::string& key);
};


#endif