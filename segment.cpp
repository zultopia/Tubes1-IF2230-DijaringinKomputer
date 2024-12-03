#include "segment.hpp"

Segment syn(Segment *segment, uint32_t seqNum)
{
    segment->flags.syn = 1;
    segment->seqNum = seqNum;
    return updateChecksum(*segment);
}

Segment ack(Segment *segment, uint32_t seqNum, uint32_t ackNum)
{
    segment->flags.ack = 1;
    segment->seqNum = seqNum;
    segment->ackNum = ackNum;
    return updateChecksum(*segment);
}

Segment synAck(Segment *segment, uint32_t seqNum, uint32_t ackNum)
{
    segment->flags.syn = 1;
    segment->flags.ack = 1;
    segment->seqNum = seqNum;
    segment->ackNum = ackNum;
    return updateChecksum(*segment);
}

Segment fin()
{
    Segment segment = {};
    segment.flags.fin = 1;
    return updateChecksum(segment);
}

Segment finAck()
{
    Segment segment = {};
    segment.flags.fin = 1;
    segment.flags.ack = 1;
    return updateChecksum(segment);
}

uint16_t calculateChecksum(Segment segment) {
    uint32_t sum = 0;

    sum += segment.sourcePort;
    sum += segment.destPort;
    sum += (segment.seqNum >> 16) & 0xFFFF;
    sum += segment.seqNum & 0xFFFF;
    sum += (segment.ackNum >> 16) & 0xFFFF;
    sum += segment.ackNum & 0xFFFF;
    sum += (segment.data_offset << 4) | segment.reserved;
    sum += (segment.flags.cwr << 15) | (segment.flags.ece << 14) | (segment.flags.urg << 13) |
           (segment.flags.ack << 12) | (segment.flags.psh << 11) | (segment.flags.rst << 10) |
           (segment.flags.syn << 9) | (segment.flags.fin << 8);
    sum += segment.window;
    sum += segment.urgentPointer;

    for (size_t i = 0; i < MAX_PAYLOAD_SIZE / 2; i++) {
        uint16_t data = (segment.payload[i * 2] << 8) | segment.payload[i * 2 + 1];
        sum += data;
    }

    sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum & 0xFFFF;
}

Segment updateChecksum(Segment segment)
{
    uint16_t checksum = calculateChecksum(segment);
    segment.checksum = checksum;
    return segment;
}

bool isValidChecksum(Segment segment)
{
    uint16_t calculatedChecksum = calculateChecksum(segment);
    return segment.checksum == calculatedChecksum;
}

