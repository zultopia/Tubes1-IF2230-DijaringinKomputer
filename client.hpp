#ifndef Client_H
#define Client_H

#include "color.hpp"
#include "tcpsocket.hpp"
#include <string>

class Client
{
private:
    string destIP;
    uint16_t destPort;
    bool sendingFile;

public:
    Client(std::string ip, int32_t port);
    ~Client();
    void run();

    void setDestination();
    void setSendingFile(bool isSendingFile);
    

private:
    TCPSocket *connection;
};

#endif