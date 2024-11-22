#ifndef SERVER_H
#define SERVER_H

#include "node.hpp"
#include "socket.hpp"
#include <string>

class Server : public Node
{
private:
    int32_t port;

public:
    Server(std::string ip, int32_t port);
    ~Server();
    void run() override;
    void handleMessage(void *buffer) override;

private:
    void handleClient(int32_t clientFd);
};

#endif // SERVER_H