#include "socket.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>

TCPSocket::TCPSocket(string ip, int32_t port) : ip(ip), port(port), status(LISTEN)
{
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
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

    segmentHandler = new SegmentHandler();
}

TCPSocket::~TCPSocket()
{
    if (socketFd != -1)
    {
        close();
    }
    delete segmentHandler;
}

void TCPSocket::listen()
{
    if (::listen(socketFd, 10) < 0)
    {
        throw runtime_error("Failed to set socket to listen state");
    }
    status = LISTEN;
    cout << "Socket is now listening on " << ip << ":" << port << endl;
}

void TCPSocket::connect(string targetIp, int32_t targetPort)
{
    struct sockaddr_in targetAddress = {};
    targetAddress.sin_family = AF_INET;
    targetAddress.sin_port = htons(targetPort);
    if (inet_pton(AF_INET, targetIp.c_str(), &targetAddress.sin_addr) <= 0)
    {
        throw runtime_error("Invalid target IP address");
    }

    if (::connect(socketFd, (struct sockaddr *)&targetAddress, sizeof(targetAddress)) < 0)
    {
        throw runtime_error("Failed to connect to target");
    }

    status = SYN_SENT;
    cout << "Connected to " << targetIp << ":" << targetPort << endl;
}

void TCPSocket::send(string targetIp, int32_t targetPort, void *dataStream, uint32_t dataSize)
{
    if (dataStream == nullptr || dataSize == 0)
    {
        throw runtime_error("Invalid data stream");
    }

    // Sending data
    ssize_t sentBytes = ::send(socketFd, dataStream, dataSize, 0);
    if (sentBytes < 0)
    {
        throw runtime_error("Failed to send data");
    }

    cout << "Sent " << sentBytes << " bytes to " << targetIp << ":" << targetPort << endl;
}

int32_t TCPSocket::recv(void *buffer, uint32_t length)
{
    if (buffer == nullptr || length == 0)
    {
        throw runtime_error("Invalid buffer");
    }

    ssize_t receivedBytes = ::recv(socketFd, buffer, length, 0);
    if (receivedBytes < 0)
    {
        throw runtime_error("Failed to receive data");
    }

    cout << "Received " << receivedBytes << " bytes" << endl;
    return receivedBytes;
}

void TCPSocket::close()
{
    if (socketFd != -1)
    {
        ::close(socketFd);
        socketFd = -1;
        status = CLOSE_WAIT;
        cout << "Socket closed" << endl;
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