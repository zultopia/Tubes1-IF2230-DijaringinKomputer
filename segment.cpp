#include "segment.hpp"

Segment syn(uint32_t seqNum)
{
    Segment segment = {};
    segment.flags.syn = 1;
    segment.seqNum = seqNum;
    return updateChecksum(segment);
}

Segment ack(uint32_t seqNum, uint32_t ackNum)
{
    Segment segment = {};
    segment.flags.ack = 1;
    segment.seqNum = seqNum;
    segment.ackNum = ackNum;
    return updateChecksum(segment);
}

Segment synAck(uint32_t seqNum, uint32_t ackNum)
{
    Segment segment = syn(seqNum);
    segment.flags.ack = 1;
    segment.ackNum = ackNum;
    return updateChecksum(segment);
}

Segment fin()
{
    Segment segment = {};
    segment.flags.fin = 1;
    return updateChecksum(segment);
}

Segment finAck()
{
    Segment segment = fin();
    segment.flags.ack = 1;
    return updateChecksum(segment);
}

uint8_t *calculateChecksum(Segment segment)
{
    // Simplified checksum calculation
    uint8_t *checksum = new uint8_t[2]{0};
    checksum[0] = (segment.seqNum & 0xFF);
    checksum[1] = ((segment.seqNum >> 8) & 0xFF);
    return checksum;
}

Segment updateChecksum(Segment segment)
{
    uint8_t *checksum = calculateChecksum(segment);
    segment.checksum = (checksum[1] << 8) | checksum[0];
    delete[] checksum;
    return segment;
}

bool isValidChecksum(Segment segment)
{
    uint8_t *calculatedChecksum = calculateChecksum(segment);
    uint16_t calculated = (calculatedChecksum[1] << 8) | calculatedChecksum[0];
    delete[] calculatedChecksum;
    return segment.checksum == calculated;
}