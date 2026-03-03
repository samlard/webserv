#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
    int _fd;                    // Socket de communication
    int _serverIndex;           // Index du serveur qui l'a accepté (-1 = inconnu)
    std::string _requestBuffer; // Données reçues (requête HTTP en cours)
    bool _requestComplete;      // Requête entièrement reçue ?
    std::string _response;      // Réponse à envoyer (si prête)

public:
    // Constructeurs
    Client();
    Client(int fd, int serverIndex);
    
    // Getters
    int getFd() const;
    int getServerIndex() const;
    const std::string& getRequestBuffer() const;
    bool isRequestComplete() const;
    const std::string& getResponse() const;
    
    // Setters / Modifiers
    void setServerIndex(int index);
    void appendToRequest(const std::string& data);
    void markRequestComplete();
    void setResponse(const std::string& response);
    void clear();  // Reset pour réutilisation ou nettoyage
};

#endif