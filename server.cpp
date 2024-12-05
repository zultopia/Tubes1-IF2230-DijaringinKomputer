#include "server.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <thread>

Server::Server(std::string ip, uint16_t port)
{
    // this->port = port;
    this->establishedIp = {0};
    this->connection = new TCPSocket(ip, port);
    this->data = "";
}

Server::~Server()
{
    connection->close();
    delete connection;
}

void Server::setData(const std::string& dt) {
    data = dt;
}

void Server::run()
{
    char buffer[1024]; // Buffer for receiving data
    std::cout << "Server is waiting for incoming data..." << std::endl;

    Segment *receivedSegment = nullptr;
    int32_t receivedBytes = 0;

    while (true)
    {
        switch (connection->getStatus())
        {
        case LISTEN:
        {
            // Wait for incoming SYN packets
            receivedBytes = connection->recv(buffer, sizeof(buffer));
            if (receivedBytes > 0)
            {
                receivedSegment = reinterpret_cast<Segment *>(buffer);

                // Validate destination port depends on TCP Header
                if (receivedSegment->destPort == connection->getPort())
                {
                    // Check if it's a SYN packet
                    if (receivedSegment->flags.syn && !receivedSegment->flags.ack)
                    {
                        std::cout << "[Handshake] [S=" << receivedSegment->seqNum << "] Receiving SYN request from " << this->connection->getSenderIp() << ":" << receivedSegment->sourcePort << std::endl;

                        // PINJAM STATE UNTUK RETRANSMIT
                        connection->setStatus(SYN_SENT);
                        connection->setRetryAttempt(0);

                        // // Prepare and send a SYN-ACK packet
                        // connection->setDataStream(nullptr); // Clear any previous data stream
                        
                        // Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);

                        // // SYN-ACK requires incrementing the received sequence number by 1
                        // Segment synAckSegment = synAck(&segment, connection->getCurrentSeqNum(), receivedSegment->seqNum + 1);

                        // connection->send(this->connection->getSenderIp(), receivedSegment->sourcePort, &synAckSegment, sizeof(synAckSegment));
                        
                        // std::cout << "[Handshake] [S=" << synAckSegment.seqNum << "] " << "[A=" << synAckSegment.ackNum << "] " <<  "Sending SYN-ACK request to " << this->connection->getSenderIp() << ":" << synAckSegment.destPort << std::endl;

                        // connection->setStatus(SYN_RECEIVED);
                        // connection->setRetryAttempt(0);

                        // break;
                    }
                }
            }

            break;
        }
        case SYN_SENT:
        {
            // Prepare and send a SYN-ACK packet
            connection->setDataStream(nullptr); // Clear any previous data stream
            
            Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);

            // SYN-ACK requires incrementing the received sequence number by 1
            Segment synAckSegment = synAck(&segment, connection->getCurrentSeqNum(), receivedSegment->seqNum + 1);

            connection->send(this->connection->getSenderIp(), receivedSegment->sourcePort, &synAckSegment, sizeof(synAckSegment));
            
            std::cout << "[Handshake] [S=" << synAckSegment.seqNum << "] " << "[A=" << synAckSegment.ackNum << "] " <<  "Sending SYN-ACK request to " << this->connection->getSenderIp() << ":" << synAckSegment.destPort << std::endl;

            connection->setStatus(SYN_RECEIVED);

            break;
        }
        case SYN_RECEIVED:
        {
            // Wait for ACK from the client
            receivedBytes = connection->recv(buffer, sizeof(buffer));
            if (receivedBytes > 0)
            {
                receivedSegment = reinterpret_cast<Segment *>(buffer);

                // Validate destination port
                if (receivedSegment->destPort == connection->getPort())
                {
                    // Check if it's an ACK packet
                    if (!receivedSegment->flags.syn && receivedSegment->flags.ack)
                    {
                        std::cout << "[Handshake] [A=" << receivedSegment->ackNum << "] Receiving ACK request from " << this->connection->getSenderIp() << ":" << receivedSegment->sourcePort << std::endl;
                        
                        connection->setCurrentSeqNum(receivedSegment->ackNum);

                        connection->setStatus(ESTABLISHED);

                        std::cout << "Connection in Server established, ready to send/receive data" << std::endl;

                        break;
                    }
                }
            }

            // Since we didn't receive the expected ACK, we will retransmit the SYN-ACK packet
            connection->setStatus(SYN_SENT);
            connection->setRetryAttempt(this->connection->getRetryAttempt() + 1);

            // std::this_thread::sleep_for(std::chrono::milliseconds(connection->getWaitRetransmitTime()));

            // // Retransmit SYN-ACK packet
            // connection->setDataStream(nullptr); // Clear any previous data stream
            // Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);

            // // SYN-ACK requires incrementing the received sequence number by 1
            // Segment synAckSegment = synAck(&segment, connection->getCurrentSeqNum(), receivedSegment->seqNum + 1);

            // connection->send(this->connection->getSenderIp(), receivedSegment->sourcePort, &synAckSegment, sizeof(synAckSegment));
            // std::cout << "Server sent SYN-ACK packet to Client." << std::endl;

            break;
        }

        case ESTABLISHED:
        {
            uint8_t* dataStream = new uint8_t[data.size() + 1];
            memcpy(dataStream, data.c_str(), data.size() + 1);

            connection->setDataStream(dataStream);

            size_t dataSize = data.size();
            size_t currentIndex = 0;
            size_t windowSize = (MAX_32_BIT - (MAX_PAYLOAD_SIZE + 1));
            size_t LAR = connection->getCurrentSeqNum();
            size_t LFS = connection->getCurrentSeqNum();
            
            connection->setCurrentSeqNum(connection->getCurrentSeqNum() + 1);

            std::vector<Segment> sentSegments;
            std::unordered_map<size_t, std::chrono::steady_clock::time_point> sentTimes;
            
            while (currentIndex < dataSize) {
                while (LFS - LAR > windowSize) {
                    std::cout << "[Established] Waiting for a free sliding window." << std::endl;

                    receivedBytes = connection->recv(buffer, sizeof(buffer));
                    if (receivedBytes > 0) {
                        receivedSegment = reinterpret_cast<Segment *>(buffer);

                        if (receivedSegment->ackNum > LAR) {
                            std::cout << "[Established] [A=" << receivedSegment->ackNum << "] ACKed" << std::endl;
                            LAR = receivedSegment->ackNum;
                        }
                    }

                    // auto now = std::chrono::steady_clock::now();
                    // for (const auto& entry : sentTimes) {
                    //     if (std::chrono::duration_cast<std::chrono::milliseconds>(now - entry.second).count() > 5000) {
                    //         size_t seqNum = entry.first;
                    //         if (seqNum > LAR) {
                    //             std::cout << "[RETRANSMIT] Retransmitting Segment [S=" << seqNum << "]" << std::endl;
                    //             connection->send(connection->getSenderIp(), connection->getPort(), &sentSegments[seqNum - 1], sizeof(Segment));
                    //             sentTimes[seqNum] = now;
                    //         }
                    //     }
                    // }
                }

                size_t remainingData = dataSize - currentIndex;
                size_t payloadSize = std::min(MAX_PAYLOAD_SIZE, remainingData);

                Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort, currentIndex);

                sentSegments.push_back(segment);
                sentTimes[segment.seqNum] = std::chrono::steady_clock::now();

                connection->send(connection->getSenderIp(), receivedSegment->sourcePort, &segment, sizeof(segment));
                std::cout << "[Established] [S=" << segment.seqNum << "] Sent" << std::endl;

                LFS = segment.seqNum;

                currentIndex += payloadSize;
                connection->setCurrentSeqNum(connection->getCurrentSeqNum() + payloadSize);
            }

            std::cout << "[Established] All packets have been sent." << std::endl;

            while (true) {
                receivedBytes = connection->recv(buffer, sizeof(buffer));
                if (receivedBytes > 0) {
                    receivedSegment = reinterpret_cast<Segment *>(buffer);

                    if (receivedSegment->ackNum > LAR) {
                        std::cout << "[Established] [A=" << receivedSegment->ackNum << "] ACKed" << std::endl;
                        LAR = receivedSegment->ackNum;

                        if (LAR == LFS) {
                            std::cout << "All packets successfully acknowledged. Closing connection." << std::endl;
                            connection->setStatus(FIN_WAIT_1);
                            break;
                        }
                    }
                }
                // auto now = std::chrono::steady_clock::now();
                // for (const auto& entry : sentTimes) {
                //     if (std::chrono::duration_cast<std::chrono::milliseconds>(now - entry.second).count() > 5000) {
                //         size_t seqNum = entry.first;
                //         if (seqNum > LAR) {
                //             std::cout << "[RETRANSMIT] Retransmitting Segment [S=" << seqNum << "]" << std::endl;
                //             connection->send(connection->getSenderIp(), connection->getPort(), &sentSegments[seqNum - 1], sizeof(Segment));
                //             sentTimes[seqNum] = now;
                //         }
                //     }
                // }
            }

            break;
        }


        case FIN_WAIT_1:
        {
            // Prepare and send a FIN packet
            connection->setDataStream(nullptr); // Clear any previous data stream
            
            Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);

            // FIN-ACK requires WITHOUT incrementing the received sequence number by 1
            Segment finSegment = fin(&segment, connection->getCurrentSeqNum(), receivedSegment->seqNum);

            connection->send(this->connection->getSenderIp(), receivedSegment->sourcePort, &finSegment, sizeof(finSegment));
            
            std::cout << "[Closing] [S=" << finSegment.seqNum << "] " << "[A=" << finSegment.ackNum << "] " <<  "Sending FIN request to " << this->connection->getSenderIp() << ":" << finSegment.destPort << std::endl;

            connection->setStatus(CLOSING);
            connection->setRetryAttempt(0);
            
            break;
        }

        case FIN_WAIT_2:
        {
            break;
        }

        case CLOSE_WAIT:
        {
            break;
        }

        case CLOSING:
        {
            // Wait for FIN from the client
            receivedBytes = connection->recv(buffer, sizeof(buffer));
            if (receivedBytes > 0)
            {
                receivedSegment = reinterpret_cast<Segment *>(buffer);

                // Validate destination port
                if (receivedSegment->destPort == connection->getPort())
                {
                    // Check if it's an FIN-ACK packet
                    if (!receivedSegment->flags.syn && receivedSegment->flags.ack && receivedSegment->flags.fin)
                    {
                        std::cout << "[CLOSING] [S=" << receivedSegment->seqNum << "] Receiving FIN-ACK request from " << this->connection->getSenderIp() << ":" << receivedSegment->sourcePort << std::endl;
                        
                        connection->setCurrentSeqNum(receivedSegment->ackNum);
                        connection->setStatus(LAST_ACK);
                        connection->setRetryAttempt(0);

                        break;
                    }
                }
            }

            connection->setStatus(FIN_WAIT_1);
            connection->setRetryAttempt(connection->getRetryAttempt() + 1);
            
            break;
        }

        case LAST_ACK:
        {
            // Prepare and send a ACK packet
            connection->setDataStream(nullptr); // Clear any previous data stream
            
            Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);

            // ACK request requires incrementing the received sequence number by 1
            Segment ackSegment = ack(&segment, connection->getCurrentSeqNum(), receivedSegment->seqNum + 1);
            
            for (int i = 0; i < connection->getMaxRetries(); ++i) {
                connection->send(this->connection->getSenderIp(), receivedSegment->sourcePort, &ackSegment, sizeof(ackSegment));
            }
            
            std::cout << "[Closing] [S=" << ackSegment.seqNum << "] " << "[A=" << ackSegment.ackNum << "] " <<  "Sending ACK request to " << this->connection->getSenderIp() << ":" << ackSegment.destPort << std::endl;

            cout << "Server Connection closed successfully." << endl;
            exit(0);
            
            break;
        }

        default:
            std::cout << "Unknown state!" << std::endl;
            break;
        }
    }
}
