#ifndef SERVER_H
#define SERVER_H

#include "tcpsocket.hpp"
#include <string>

class Server
{
private:
    int32_t port;

public:
    Server(std::string ip, int32_t port);
    ~Server();
    void run();

private:
    TCPSocket *connection;
};

#endif // SERVER_H