#ifndef CLIENT_H
#define CLIENT_H

#include "node.hpp"
#include "socket.hpp"
#include <string>

class Client : public Node
{
private:
    std::string targetIp;
    int32_t targetPort;

public:
    Client(std::string targetIp, int32_t targetPort);
    ~Client();
    void run() override;
    void handleMessage(void *buffer) override;

private:
    void sendMessage(const std::string &message);
};

#endif // CLIENT_H