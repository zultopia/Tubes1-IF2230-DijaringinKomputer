#include "client.hpp"
#include <iostream>
#include <unistd.h>

Client::Client(std::string targetIp, int32_t targetPort)
{
    this->targetIp = targetIp;
    this->targetPort = targetPort;
    this->connection = new TCPSocket(targetIp, targetPort);
}

Client::~Client()
{
    delete connection;
}

void Client::run()
{
    connection->connect(targetIp, targetPort);

    std::cout << "Client connected to server at " << targetIp << ":" << targetPort << std::endl;

    std::string message = "Hello from Client!";
    sendMessage(message);

    char buffer[1024];
    int bytesRead = connection->recv(buffer, sizeof(buffer));
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        handleMessage(buffer);
    }

    connection->close();
}

void Client::handleMessage(void *buffer)
{
    std::string message = static_cast<char *>(buffer);
    std::cout << "Client received message: " << message << std::endl;
}

void Client::sendMessage(const std::string &message)
{
    connection->send(targetIp, targetPort, (void *)message.c_str(), message.size());
    std::cout << "Client sent message: " << message << std::endl;
}