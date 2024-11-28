#ifndef Client_H
#define Client_H

#include "tcpsocket.hpp"
#include <string>

class Client
{
private:
    int32_t port;

public:
    Client(std::string ip, int32_t port);
    ~Client();
    void run();

private:
    TCPSocket *connection;
};

#endif