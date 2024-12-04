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
}

Server::~Server()
{
    connection->close();
    delete connection;
}

void Server::run()
{
    std::string message = "Hello, Server!";


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

                        // Prepare and send a SYN-ACK packet
                        connection->setDataStream(nullptr); // Clear any previous data stream
                        Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);

                        // SYN-ACK requires incrementing the received sequence number by 1
                        Segment synAckSegment = synAck(&segment, connection->getCurrentSeqNum(), receivedSegment->seqNum + 1);

                        connection->send(this->connection->getSenderIp(), receivedSegment->sourcePort, &synAckSegment, sizeof(synAckSegment));
                        
                        std::cout << "[Handshake] [S=" << synAckSegment.seqNum << "] " << "[A=" << synAckSegment.ackNum << "] " <<  "Sending SYN-ACK request to " << this->connection->getSenderIp() << ":" << synAckSegment.destPort << std::endl;

                        connection->setStatus(SYN_RECEIVED);
                    }
                }
            }

            break;
        }
        case SYN_SENT:
        {
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

            std::this_thread::sleep_for(std::chrono::milliseconds(connection->getWaitRetransmitTime()));

            // Retransmit SYN-ACK packet
            connection->setDataStream(nullptr); // Clear any previous data stream
            Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);

            // SYN-ACK requires incrementing the received sequence number by 1
            Segment synAckSegment = synAck(&segment, connection->getCurrentSeqNum(), receivedSegment->seqNum + 1);

            connection->send(this->connection->getSenderIp(), receivedSegment->sourcePort, &synAckSegment, sizeof(synAckSegment));
            std::cout << "Server sent SYN-ACK packet to Client." << std::endl;
            break;
        }

        case ESTABLISHED:
        {
            std::string data = "I'm in(ooh-ooh) Woah-oh-oh (nah-nah-nah-nah, ayy, ayy) This is how the story goes (nah, nah) Woah-oh-oh I guess this is how the story goes";        
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
                            connection->setStatus(CLOSE_WAIT);
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
            break;
        }

        case FIN_WAIT_2:
        {
            break;
        }

        case CLOSE_WAIT:
        {
            // std::cout << "Server is in CLOSE_WAIT state." << std::endl;

            // // Wait for an ACK from the client to complete the connection termination
            // receivedBytes = connection->recv(buffer, sizeof(buffer));
            // if (receivedBytes > 0)
            // {
            //     receivedSegment = reinterpret_cast<Segment *>(buffer);

            //     // Send an ACK to acknowledge the FIN from the client
            //     if (receivedSegment->flags.fin)
            //     {
            //         std::cout << "[FIN Received] Sending ACK and transitioning to LAST_ACK state." << std::endl;
            //         Segment ackSegment;
            //         ackSegment.seqNum = connection->getCurrentSeqNum();
            //         ackSegment.ackNum = connection->getCurrentAckNum();
            //         ackSegment.flags.ack = true;

            //         connection->send(connection->getSenderIp(), this->destPort, &ackSegment, sizeof(ackSegment));

            //         // Transition to LAST_ACK
            //         this->state = LAST_ACK;
            //     }
            // }
            break;
        }

        case CLOSING:
        {
            break;
        }

        case LAST_ACK:
        {
            // std::cout << "Server is in LAST_ACK state." << std::endl;

            // // Wait for the final ACK from the client after sending the FIN
            // receivedBytes = connection->recv(buffer, sizeof(buffer));
            // if (receivedBytes > 0)
            // {
            //     receivedSegment = reinterpret_cast<Segment *>(buffer);

            //     // If an ACK is received, we can safely close the connection
            //     if (receivedSegment->flags.ack)
            //     {
            //         std::cout << "[ACK Received] Closing the connection." << std::endl;
            //         // Close connection or perform any final cleanup
            //         connection->close();
            //         exit(0); // Exit since the connection is closed
            //     }
            // }
            break;
        }

        default:
            std::cout << "Unknown state!" << std::endl;
            break;
        }
    }
}
