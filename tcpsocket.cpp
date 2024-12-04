#include "tcpsocket.hpp"
#include <fstream>
#include <random>
#include <iostream>
#include <stdexcept>
#include <cstring>

TCPSocket::TCPSocket(std::string ip, uint16_t port)
    : ip(ip),
      port(port),
      status(LISTEN),
      currentSeqNum(generateRandomSeqNum()),
      currentAckNum(0),
      dataStream(nullptr),
      retryAttempt(0)
{
    // Create a socket with UDP (SOCK_DGRAM)
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd == -1)
    {
        throw std::runtime_error("Failed to create socket");
    }

    // Set socket to non-blocking mode
    int flags = fcntl(socketFd, F_GETFL, 0);
    if (flags == -1 || fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        throw std::runtime_error("Failed to set socket to non-blocking mode");
    }

    // Bind the socket to the specified IP address and port
    struct sockaddr_in address = {};
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0)
    {
        throw std::runtime_error("Invalid IP address");
    }

    if (bind(socketFd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        throw std::runtime_error("Failed to bind socket");
    }

    status = LISTEN;
    std::cout << "Socket is now listening on " << ip << ":" << port << std::endl;
}

TCPSocket::~TCPSocket()
{
    if (socketFd != -1)
    {
        close();
    }
    // delete segmentHandler;
}

void TCPSocket::send(string destIp, uint16_t destPort, void *packet, uint32_t packetSize)
{
    if (this->getRetryAttempt() >= this->getMaxRetries())
    {
        throw std::runtime_error("Max retries reached");
        exit(1);
    }

    if (packet == nullptr || packetSize == 0)
    {
        throw runtime_error("Invalid packet");
    }

    struct sockaddr_in targetAddress = {};
    targetAddress.sin_family = AF_INET;
    targetAddress.sin_port = htons(destPort);
    if (inet_pton(AF_INET, destIp.c_str(), &targetAddress.sin_addr) <= 0)
    {
        throw runtime_error("Invalid target IP address");
    }

    if (this->getRetryAttempt() != 0){
        std::cout << "Retransmitting packet " << this->getRetryAttempt() << "/" << this->getMaxRetries() << std::endl;
    }

    ssize_t sentBytes = ::sendto(socketFd, packet, packetSize, 0, (struct sockaddr *)&targetAddress, sizeof(targetAddress));
    if (sentBytes < 0)
    {
        throw runtime_error("Failed to send data");
    }
}

// Receive data (non-blocking)
int32_t TCPSocket::recv(void *buffer, uint32_t length)
{
    // sleep 500ms
    usleep(300000);
    if (buffer == nullptr || length == 0)
    {
        throw std::runtime_error("Invalid buffer");
    }

    socklen_t senderAddressLength = sizeof(senderAddress);

    ssize_t receivedBytes = ::recvfrom(socketFd, buffer, length, 0,
                                       reinterpret_cast<struct sockaddr *>(&senderAddress),
                                       &senderAddressLength);

    if (receivedBytes < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // No data available in non-blocking mode
            return 0;
        }
        else
        {
            throw std::runtime_error("Failed to receive data");
        }
    }

    return receivedBytes;
}

// Utility to check if data is available
bool TCPSocket::isDataAvailable()
{
    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(socketFd, &readFds);

    struct timeval timeout = {0, 0}; // No timeout (immediate check)

    int result = select(socketFd + 1, &readFds, nullptr, nullptr, &timeout);

    if (result < 0)
    {
        throw std::runtime_error("Error during select");
    }

    return FD_ISSET(socketFd, &readFds);
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

uint32_t TCPSocket::generateRandomSeqNum()
{
    uint32_t seqNum;
    
    // Open /dev/urandom to generate a cryptographically secure random number
    std::ifstream urandom("/dev/urandom", std::ios::in | std::ios::binary);
    if (!urandom)
    {
        throw std::runtime_error("Failed to open /dev/urandom");
    }

    // Read 4 bytes from /dev/urandom to generate a random uint32_t
    urandom.read(reinterpret_cast<char*>(&seqNum), sizeof(seqNum));
    if (!urandom)
    {
        throw std::runtime_error("Failed to read random data from /dev/urandom");
    }

    urandom.close();

    return seqNum;
}

void TCPSocket::setDataStream(uint8_t *dataStream)
{
    if (dataStream != nullptr)
    {
        // Set the data stream
        this->dataStream = static_cast<void *>(dataStream);
    }
    else
    {
        // Clear the data stream
        this->dataStream = nullptr;
    }
    // this->dataSize = dataSize;
    // this->dataIndex = 0;
    // generate segment from data stream
}

Segment TCPSocket::generateSegmentsFromPayload(uint16_t destPort, size_t offset)
{
    // Create an empty segment
    Segment segment = {};
    segment.sourcePort = port;     // Local port
    segment.destPort = destPort;   // Destination port passed as an argument
    segment.seqNum = currentSeqNum;
    segment.ackNum = currentAckNum;
    segment.flags = {0};           // No flags set initially

    // Ensure dataStream is not nullptr and the offset is within a valid range
    if (dataStream != nullptr)
    {
        segment.flags.ack = 1;
        size_t dataLength = strlen(reinterpret_cast<const char*>(dataStream));

        size_t startByte = offset;

        if (startByte < dataLength)
        {
            size_t bytesToCopy = std::min(MAX_PAYLOAD_SIZE, dataLength - startByte);
            memcpy(segment.payload, reinterpret_cast<uint8_t*>(dataStream) + startByte, bytesToCopy);
        }
    }

    // Set the checksum
    segment = updateChecksum(segment);

    return segment;
}



TCPStatusEnum TCPSocket::getStatus()
{
    return status;
}

void TCPSocket::setStatus(TCPStatusEnum status)
{
    this->status = status;
}

uint32_t TCPSocket::getCurrentSeqNum()
{
    return currentSeqNum;
}

uint32_t TCPSocket::getCurrentAckNum()
{
    return currentAckNum;
}

void TCPSocket::setCurrentSeqNum(uint32_t seqNum)
{
    currentSeqNum = seqNum;
}

void TCPSocket::setCurrentAckNum(uint32_t ackNum)
{
    currentAckNum = ackNum;
}

int32_t TCPSocket::getSocket() const
{
    return socketFd;
}

string TCPSocket::getIp()
{
    return ip;
}

uint16_t TCPSocket::getPort()
{
    return port;
}

string TCPSocket::getSenderIp() const
{
    char ipBuffer[INET_ADDRSTRLEN]; // Buffer untuk alamat IP
    if (inet_ntop(AF_INET, &senderAddress.sin_addr, ipBuffer, INET_ADDRSTRLEN) == nullptr)
    {
        throw std::runtime_error("Failed to convert sender IP to string");
    }
    return std::string(ipBuffer); // Kembalikan dalam format string
}

uint32_t TCPSocket::getWaitRetransmitTime()
{
    return WAIT_RETRANSMIT_TIME;
}

uint32_t TCPSocket::getMaxRetries()
{
    return MAX_RETRIES;
}

void TCPSocket::setRetryAttempt(uint32_t retryAttempt){
    this->retryAttempt = retryAttempt;
}

uint32_t TCPSocket::getRetryAttempt(){
    return retryAttempt;
}
