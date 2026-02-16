#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <string.h>
#include <iostream>

class Config {
	private :
		int port;                    // j'en ai besoin pour bind()
    	std::string host;            // adresse ip (default: "0.0.0.0")


	public :
		Config();
		~Config();
		int parseFile(const std::string& filename);
		std::string getError();
		//getter de port pour moi dans initserver 
};




#endif