#include "tcpsocket.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>

TCPSocket::TCPSocket(string ip, int32_t port) : ip(ip), port(port), status(LISTEN)
{
    // Create a socket with UDP (SOCK_DGRAM)
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd == -1)
    {
        throw runtime_error("Failed to create socket");
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0)
    {
        throw runtime_error("Invalid IP address");
    }

    if (bind(socketFd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        throw runtime_error("Failed to bind socket");
    }

    status = LISTEN;
    std::cout << "Socket is now listening on " << ip << ":" << port << std::endl;

    // segmentHandler = new SegmentHandler();
}

TCPSocket::~TCPSocket()
{
    if (socketFd != -1)
    {
        close();
    }
    // delete segmentHandler;
}

void TCPSocket::send(string targetIp, int32_t targetPort, void *dataStream, uint32_t dataSize)
{
    if (dataStream == nullptr || dataSize == 0)
    {
        throw runtime_error("Invalid data stream");
    }

    struct sockaddr_in targetAddress = {};
    targetAddress.sin_family = AF_INET;
    targetAddress.sin_port = htons(targetPort);
    if (inet_pton(AF_INET, targetIp.c_str(), &targetAddress.sin_addr) <= 0)
    {
        throw runtime_error("Invalid target IP address");
    }

    ssize_t sentBytes = ::sendto(socketFd, dataStream, dataSize, 0, (struct sockaddr *)&targetAddress, sizeof(targetAddress));
    if (sentBytes < 0)
    {
        throw runtime_error("Failed to send data");
    }

    std::cout << "Sent " << sentBytes << " bytes to " << targetIp << ":" << targetPort << std::endl;
}

int32_t TCPSocket::recv(void *buffer, uint32_t length)
{
    if (buffer == nullptr || length == 0)
    {
        throw runtime_error("Invalid buffer");
    }

    struct sockaddr_in senderAddress = {};
    socklen_t senderAddressLength = sizeof(senderAddress);
    ssize_t receivedBytes = ::recvfrom(socketFd, buffer, length, 0, (struct sockaddr *)&senderAddress, &senderAddressLength);
    if (receivedBytes < 0)
    {
        throw runtime_error("Failed to receive data");
    }

    std::cout << "Received " << receivedBytes << " bytes" << std::endl;
    return receivedBytes;
}

void TCPSocket::close()
{
    if (socketFd != -1)
    {
        ::close(socketFd);
        socketFd = -1;
        status = LAST_ACK;
        std::cout << "Socket closed" << std::endl;
    }
}

TCPStatusEnum TCPSocket::getStatus()
{
    return status;
}

int32_t TCPSocket::getSocket() const
{
    return socketFd;
}

string TCPSocket::getIp()
{
    return ip;
}

int32_t TCPSocket::getPort()
{
    return port;
}
