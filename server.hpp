#ifndef SERVER_H
#define SERVER_H

#include "tcpsocket.hpp"
#include <string>

class Server
{
private:
    // int32_t port;
    std::string establishedIp;


public:
    Server(std::string ip, uint16_t port);
    ~Server();
    void run();

private:
    TCPSocket *connection;
};

#endif // SERVER_H