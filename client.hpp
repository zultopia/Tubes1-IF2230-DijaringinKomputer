#ifndef Client_H
#define Client_H

#include "tcpsocket.hpp"
#include <string>

class Client
{
private:
    string destIP;
    uint16_t destPort;

public:
    Client(std::string ip, int32_t port);
    ~Client();
    void run();

    void setDestination();
    

private:
    TCPSocket *connection;
};

#endif