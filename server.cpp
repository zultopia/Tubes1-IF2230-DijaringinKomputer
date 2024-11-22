#include "server.hpp"
#include <iostream>
#include <thread>
#include <unistd.h>

Server::Server(std::string ip, int32_t port)
{
    this->port = port;
    this->connection = new TCPSocket(ip, port);
}

Server::~Server()
{
    delete connection;
}

void Server::run()
{
    connection->listen(); 

    std::cout << "Server listening on port " << port << "..." << std::endl;
    while (true)
    {
        int32_t clientFd = accept(connection->getSocket(), NULL, NULL);
        if (clientFd == -1)
        {
            std::cerr << "Failed to accept connection!" << std::endl;
            continue;
        }

        std::cout << "New connection accepted!" << std::endl;

        std::thread clientHandler(&Server::handleClient, this, clientFd);
        clientHandler.detach();
    }
}

void Server::handleMessage(void *buffer)
{
    std::string message = static_cast<char *>(buffer);
    std::cout << "Server received message: " << message << std::endl;
}

void Server::handleClient(int32_t clientFd)
{
    char buffer[1024];
    while (true)
    {
        int bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0)
        {
            std::cerr << "Connection closed or error while receiving data!" << std::endl;
            break;
        }

        buffer[bytesRead] = '\0'; 
        handleMessage(buffer);     
    }

    close(clientFd); 
}