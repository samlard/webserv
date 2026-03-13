#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "Request.hpp"
#include "Response.hpp"

class Client {

private:

    int _fd;
    int _serverIndex;

    std::string _requestBuffer;
    bool _requestComplete;

    Request _request;
    Response _response;

public:

    Client();
    Client(int fd, int serverIndex);

    int getFd() const;
    int getServerIndex() const;

    std::string& getBuffer();
    void appendToBuffer(const std::string& data);
    Request& getRequest();
    Response& getResponse();

    void markRequestComplete();
    bool isRequestComplete() const;

    void clear();
};

#endif