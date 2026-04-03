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

    std::string _responseStr;
    size_t _sendOffset;

public:

    Client();
    Client(int fd, int serverIndex);

    int getFd() const;
    int getServerIndex() const;
    void setServerIndex(int idx);

    std::string& getBuffer();
    void appendToBuffer(const std::string& data);
    Request& getRequest();
    Response& getResponse();

    void markRequestComplete();
    bool isRequestComplete() const;

    void prepareResponse();
    const std::string& getResponseStr() const;
    size_t getSendOffset() const;
    void advanceSendOffset(size_t n);
    bool isSendComplete() const;

    void clear();
};

#endif