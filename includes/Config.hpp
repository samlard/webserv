#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <string.h>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <vector>

class Config {
	private :
   	 	int _port;
    	std::string _host;  // adresse ip (default: "0.0.0.0")
    	std::string _root;
    	std::string _index;          


	public :
		Config();
		~Config();
		int parseFile(const std::string& filename);
		std::string getError() const;
		int getPort() const;
    	std::string getHost() const;
    	std::string getRoot() const;
    	std::string getIndex() const;
};




#endif