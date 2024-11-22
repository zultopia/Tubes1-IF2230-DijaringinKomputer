#include "segment_handler.hpp"
#include <cstring>

SegmentHandler::SegmentHandler()
    : segmentBuffer(nullptr), windowSize(0), currentSeqNum(0), currentAckNum(0), dataStream(nullptr), dataSize(0), dataIndex(0) {}

SegmentHandler::~SegmentHandler()
{
    if (segmentBuffer != nullptr)
    {
        delete[] segmentBuffer;
    }
}

void SegmentHandler::setDataStream(uint8_t *dataStream, uint32_t dataSize)
{
    this->dataStream = dataStream;
    this->dataSize = dataSize;
    this->dataIndex = 0;
}

uint8_t SegmentHandler::getWindowSize()
{
    return this->windowSize;
}

Segment *SegmentHandler::advanceWindow(uint8_t size)
{
    if (segmentBuffer != nullptr)
    {
        delete[] segmentBuffer;
    }

    segmentBuffer = new Segment[size];

    for (uint8_t i = 0; i < size && dataIndex < dataSize; ++i)
    {
        Segment segment = {};
        segment.seqNum = currentSeqNum++;
        segment.payload = &dataStream[dataIndex];
        segment.window = windowSize;

        segmentBuffer[i] = segment;

        dataIndex++;
    }

    return segmentBuffer;
}