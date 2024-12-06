#ifndef SERVER_H
#define SERVER_H

#include "color.hpp"
#include "tcpsocket.hpp"
#include <string>

class Server
{
private:
    // int32_t port;
    std::string establishedIp;
    std::vector<uint8_t> data;
    bool sendingFile;
    std::string fileName;

public:
    Server(std::string ip, uint16_t port);
    ~Server();
    void setData(const std::string& data);
    void setData(const std::vector<uint8_t>& binaryData);
    void setSendingFile(bool isSendingFile);
    void setFileName(const std::string& name);
    void run();

private:
    TCPSocket *connection;
};

#endif // SERVER_H