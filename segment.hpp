#ifndef segment_h
#define segment_h

#include <cstdint>
#include <string>

const size_t MAX_PAYLOAD_SIZE = 1460;
const size_t MAX_32_BIT = 4294967295;

struct Segment
{
    uint16_t sourcePort : 16;
    uint16_t destPort : 16;
    uint32_t seqNum : 32;
    uint32_t ackNum : 32;

    struct
    {
        uint8_t data_offset : 4;
        uint8_t reserved : 4;
    };

    struct
    {
        uint8_t cwr : 1;
        uint8_t ece : 1;
        uint8_t urg : 1;
        uint8_t ack : 1;
        uint8_t psh : 1;
        uint8_t rst : 1;
        uint8_t syn : 1;
        uint8_t fin : 1;
    } flags;

    uint16_t window;
    uint16_t checksum;
    uint16_t urgentPointer;

    // New field for options
    struct
    {
        uint8_t sendingFile : 1; // Indicates whether the connection is for file transfer
        uint8_t reserved : 7;    // Reserved bits for future options
    } options;

    uint8_t payload[MAX_PAYLOAD_SIZE];

    bool operator<(const Segment &other) const {
        return seqNum < other.seqNum;
    }
} __attribute__((packed));


struct ReceivedPacket
{
    std::string senderIP; // Store sender's IP
    int32_t receivedBytes; // Number of bytes received
};

const uint8_t FIN_FLAG = 1;
const uint8_t SYN_FLAG = 2;
const uint8_t ACK_FLAG = 16;
const uint8_t SYN_ACK_FLAG = SYN_FLAG | ACK_FLAG;
const uint8_t FIN_ACK_FLAG = FIN_FLAG | ACK_FLAG;

/**
 * Generate Segment that contain SYN packet
 */
Segment syn(Segment *segment, uint32_t seqNum);

/**
 * Generate Segment that contain ACK packet
 */
Segment ack(Segment *segment, uint32_t seqNum, uint32_t ackNum);

/**
 * Generate Segment that contain SYN-ACK packet
 */
Segment synAck(Segment *segment, uint32_t seqNum, uint32_t ackNum, bool isSendingFile);

/**
 * Generate Segment that contain FIN packet
 */
Segment fin(Segment *segment, uint32_t seqNum, uint32_t ackNum);

/**
 * Generate Segment that contain FIN-ACK packet
 */
Segment finAck(Segment *segment, uint32_t seqNum, uint32_t ackNum);

// update return type as needed
uint16_t calculateChecksum(Segment segment);

/**
 * Return a new segment with a calcuated checksum fields
 */
Segment updateChecksum(Segment segment);

/**
 * Check if a TCP Segment has a valid checksum
 */
bool isValidChecksum(Segment segment);

#endif