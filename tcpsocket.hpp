#ifndef tcpsocket_h
#define tcpsocket_h

#include "color.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <functional>
#include "segment.hpp"
#include <fcntl.h>
#include <netinet/in.h>

using namespace std;

// for references
// https://maxnilz.com/docs/004-network/003-tcp-connection-state/
// You can use a different state enum if you'd like
enum TCPStatusEnum
{
    LISTEN = 0,
    SYN_SENT = 1,
    SYN_RECEIVED = 2,
    ESTABLISHED = 3,
    FIN_WAIT_1 = 4,
    FIN_WAIT_2 = 5,
    CLOSE_WAIT = 6,
    CLOSING = 7,
    LAST_ACK = 8
};

class TCPSocket
{
    // todo add tcp connection state?
private:
    static constexpr uint32_t WAIT_RETRANSMIT_TIME = 500; // 500 milliseconds
    static constexpr uint32_t MAX_RETRIES = 10;
    /**
     * The ip address and port for the socket instance
     * Not to be confused with ip address and port of the connected connection
     */
    string ip;
    int32_t port;

    /**
     * Socket descriptor
     */
    int32_t socketFd; 

    // SegmentHandler *segmentHandler;

    TCPStatusEnum status;

    struct sockaddr_in senderAddress;

    uint32_t currentSeqNum;
    uint32_t currentAckNum;
    void *dataStream;
    size_t dataSize;     

    uint32_t generateRandomSeqNum();

    uint32_t retryAttempt;
public:
    TCPSocket(string ip, uint16_t port);
    ~TCPSocket();
    void send(string destIp, uint16_t destPort, void *dataStream, uint32_t dataSize);
    int32_t recv(void *buffer, uint32_t length, size_t duration = 300000);
    bool isDataAvailable();
    void close();

    TCPStatusEnum getStatus();
    void setStatus(TCPStatusEnum status);
    uint32_t getCurrentSeqNum();
    uint32_t getCurrentAckNum();
    void setCurrentSeqNum(uint32_t seqNum);
    void setCurrentAckNum(uint32_t ackNum);
    int32_t getSocket() const;
    string getIp();
    uint16_t getPort();
    string getSenderIp() const;
    uint32_t getWaitRetransmitTime();
    uint32_t getMaxRetries();
    uint32_t getRetryAttempt();
    void setRetryAttempt(uint32_t retryAttempt);

    Segment generateSegmentsFromPayload(uint16_t destPort, size_t offset = 0);
    void setDataStream(uint8_t *dataStream, size_t dataSize);
};

#endif