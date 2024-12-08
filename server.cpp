#include "server.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <map>

Server::Server(std::string ip, uint16_t port)
{
    // this->port = port;
    this->establishedIp = {0};
    this->connection = new TCPSocket(ip, port);
    this->data = std::vector<uint8_t>();
    this->sendingFile = false;
    std::srand(static_cast<unsigned int>(std::time(0)));
}

Server::~Server()
{
    connection->close();
    delete connection;
}

void Server::setFileName(const std::string& name) {
    fileName = name;
}

void Server::setSendingFile(bool isSendingFile){
    sendingFile = isSendingFile? 1: 0;
}

void Server::setData(const std::string& textData) 
{
    data.assign(textData.begin(), textData.end()); 
}

void Server::setData(const std::vector<uint8_t>& binaryData) {
    data = binaryData; 
}

void Server::run()
{
    char buffer[1024]; // Buffer for receiving data
    std::cout <<  Color::color("[~] Server is waiting for incoming data...", Color::MAGENTA) << std::endl;

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
                        connection->setCurrentAckNum(receivedSegment->seqNum + 1);

                        std::cout << Color::color("[+] [Handshake]", Color::GREEN) << "[S=" << receivedSegment->seqNum << "] Receiving SYN request from " << this->connection->getSenderIp() << ":" << receivedSegment->sourcePort << std::endl;

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
            connection->setDataStream(nullptr, 0); // Clear any previous data stream
            
            Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);

            // SYN-ACK requires incrementing the received sequence number by 1            
            Segment synAckSegment = synAck(&segment, connection->getCurrentSeqNum(), connection->getCurrentAckNum(), sendingFile);

            connection->send(this->connection->getSenderIp(), receivedSegment->sourcePort, &synAckSegment, sizeof(synAckSegment));
            
            std::cout << Color::color("[i] [Handshake]", Color::YELLOW) << "[S=" << synAckSegment.seqNum << "] " << "[A=" << synAckSegment.ackNum << "] " <<  "Sending SYN-ACK request to " << this->connection->getSenderIp() << ":" << synAckSegment.destPort << std::endl;

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
                        connection->setCurrentSeqNum(receivedSegment->ackNum);

                        std::cout << Color::color("[i] [Handshake]", Color::YELLOW) << "[A=" << receivedSegment->ackNum << "] Receiving ACK request from " << this->connection->getSenderIp() << ":" << receivedSegment->sourcePort << std::endl;

                        connection->setStatus(ESTABLISHED);

                        std::cout << Color::color("Connection in Server established, ready to send/receive data", Color::YELLOW) << std::endl;

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
            if (sendingFile) {
                std::string metadata = "FILENAME:" + fileName + ";SIZE:" + std::to_string(data.size()) + ";";
                std::vector<uint8_t> metadataBytes(metadata.begin(), metadata.end());
                connection->setDataStream(metadataBytes.data(), metadataBytes.size());
                Segment metadataSegment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);
                Segment ackSegment = ack(&metadataSegment, connection->getCurrentSeqNum(), connection->getCurrentAckNum());

                connection->send(connection->getSenderIp(), receivedSegment->sourcePort, &metadataSegment, sizeof(metadataSegment));
                std::cout << Color::color("[i] [ESTABLISHED] Metadata sent: ", Color::YELLOW) << metadata << " [S="<< ackSegment.seqNum <<"]" << std::endl;

                bool metadataAcked = false;
                while (!metadataAcked) {
                    receivedBytes = connection->recv(buffer, sizeof(buffer));
                    if (receivedBytes > 0) {
                        receivedSegment = reinterpret_cast<Segment*>(buffer);
                        std::cout << receivedSegment->ackNum << " expecting " << metadataSegment.seqNum + metadata.size() << std::endl;
                        if (receivedSegment->flags.ack) {
                            std::cout << Color::color("[+] [ESTABLISHED] Metadata ACK received.", Color::GREEN) << std::endl;
                            metadataAcked = true;
                        }
                    }
                }
            }
            // uint8_t* dataStream = new uint8_t[data.size() + 1];
            // memcpy(dataStream, data.c_str(), data.size() + 1);

            // connection->setDataStream(dataStream);
            connection->setDataStream(data.data(), data.size());

            size_t dataSize = data.size();
            size_t windowSize = (MAX_32_BIT - (MAX_PAYLOAD_SIZE + 1));
            size_t LAR = connection->getCurrentSeqNum();
            size_t LFS = connection->getCurrentSeqNum();
            
            connection->setCurrentSeqNum(connection->getCurrentSeqNum() + 1);

            std::map<size_t, std::chrono::steady_clock::time_point> sentTimes; // size_t is seqNum of sent segments 

            size_t currentIndex = 0;
            size_t startingSeqNum = connection->getCurrentSeqNum();
            while (true) {
                while (currentIndex < dataSize) {
                    while (LFS - LAR > windowSize) {
                        std::cout << Color::color("[~] [Established]", Color::MAGENTA) << "Waiting for a free sliding window." << std::endl;

                        receivedBytes = connection->recv(buffer, sizeof(buffer));
                        while (receivedBytes > 0) {
                            receivedSegment = reinterpret_cast<Segment *>(buffer);

                            if (receivedSegment->seqNum > LAR) {
                                std::cout << Color::color("[i] [Established]", Color::GREEN) <<" [A=" << receivedSegment->ackNum << "] ACKed" << std::endl;
                                LAR = receivedSegment->seqNum;
                                sentTimes.erase(receivedSegment->seqNum);
                            }

                            receivedBytes = connection->recv(buffer, sizeof(buffer));
                        }
                    }

                    auto now = std::chrono::steady_clock::now();
                    for (const auto& entry : sentTimes) {
                        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - entry.second).count() > connection->getWaitRetransmitTime()) {
                            std::cout << Color::color("[i] [Established]", Color::YELLOW) <<" [S=" << entry.first << "] not ACKed in time. Retransmitting." << std::endl;
                            currentIndex = entry.first - startingSeqNum;
                            connection->setCurrentSeqNum(entry.first);
                            LFS = connection->getCurrentSeqNum() - MAX_PAYLOAD_SIZE;
                            LAR = connection->getCurrentSeqNum() - MAX_PAYLOAD_SIZE;
                            sentTimes.clear();
                            break;
                        }
                    }

                    size_t remainingData = dataSize - currentIndex;
                    size_t payloadSize = std::min(MAX_PAYLOAD_SIZE, remainingData);

                    Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort, currentIndex);
                    if (currentIndex + MAX_PAYLOAD_SIZE >= dataSize) {
                        segment.flags.fin = 1;
                        segment = updateChecksum(segment);
                    }

                    sentTimes[segment.seqNum] = std::chrono::steady_clock::now();

                    // // Generate a random integer between 0 and RAND_MAX
                    // int random_int = std::rand();

                    // // Scale it to a probability between 0 and 1
                    // double ploss = static_cast<double>(random_int) / RAND_MAX;

                    // std::cout << ploss;                    
                    // if (ploss < 0.9) {
                        connection->send(connection->getSenderIp(), receivedSegment->sourcePort, &segment, sizeof(segment));
                    // } else {
                    //     std::cout << Color::color("[i] [Established]", Color::YELLOW) <<" [S=" << segment.seqNum << "] NOT Sent" << std::endl;
                    //     ploss -= 0.011;
                    // }
                    
                    std::cout << Color::color("[i] [Established]", Color::YELLOW) <<" [S=" << segment.seqNum << "] Sent" << std::endl;

                    LFS = segment.seqNum;

                    currentIndex += payloadSize;
                    connection->setCurrentSeqNum(connection->getCurrentSeqNum() + payloadSize);
                }

                std::cout << Color::color("[~] [Established] All packets have been sent. Waiting for ACKs", Color::MAGENTA) << std::endl;

                bool needToRetransmit = false;
                bool allAcked = false;
                while (!needToRetransmit && !allAcked) {
                    receivedBytes = connection->recv(buffer, sizeof(buffer));
                    while (receivedBytes > 0) {
                        receivedSegment = reinterpret_cast<Segment *>(buffer);
                        if (receivedSegment->seqNum > LAR) {
                            std::cout << Color::color("[i] [Established]", Color::GREEN) <<"[A=" << receivedSegment->ackNum << "] ACKed" << std::endl;
                            LAR = receivedSegment->seqNum;
                            sentTimes.erase(receivedSegment->seqNum);

                            if (LAR == LFS) {
                                std::cout << Color::color("[i] All packets successfully acknowledged. Closing connection.", Color::YELLOW) << std::endl;

                                connection->setStatus(FIN_WAIT_1);
                                allAcked = true;
                                break;
                            }
                        }
                        receivedBytes = connection->recv(buffer, sizeof(buffer));
                    }
                    
                    auto now = std::chrono::steady_clock::now();
                    for (const auto& entry : sentTimes) {
                        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - entry.second).count() > connection->getWaitRetransmitTime()) {
                            std::cout << Color::color("[i] [Established]", Color::YELLOW) <<" [S=" << entry.first << "] not ACKed in time. Retransmitting." << std::endl; 
                            currentIndex = entry.first - startingSeqNum;
                            connection->setCurrentSeqNum(entry.first);
                            LFS = connection->getCurrentSeqNum() - MAX_PAYLOAD_SIZE;
                            LAR = connection->getCurrentSeqNum() - MAX_PAYLOAD_SIZE;
                            needToRetransmit = true;
                            sentTimes.clear();
                            break;
                        }
                    }
                }

                if (!needToRetransmit) {
                    break;
                }
            }

            break;
        }


        case FIN_WAIT_1:
        {
            // Prepare and send a FIN packet
            connection->setDataStream(nullptr, 0); // Clear any previous data stream
            
            Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);

            // FIN-ACK requires WITHOUT incrementing the received sequence number by 1
            Segment finSegment = fin(&segment, connection->getCurrentSeqNum(), connection->getCurrentAckNum());

            connection->send(this->connection->getSenderIp(), receivedSegment->sourcePort, &finSegment, sizeof(finSegment));
            
            std::cout << Color::color("[i] [Closing]", Color::YELLOW) <<"[S=" << finSegment.seqNum << "] " << "[A=" << finSegment.ackNum << "] " <<  "Sending FIN request to " << this->connection->getSenderIp() << ":" << finSegment.destPort << std::endl;

            connection->setStatus(CLOSING);
            
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
                        connection->setCurrentSeqNum(receivedSegment->ackNum);
                        connection->setCurrentAckNum(receivedSegment->seqNum + 1);

                        std::cout << Color::color("[+] [Closing]", Color::GREEN) << " [S=" << receivedSegment->seqNum << "] Receiving FIN-ACK request from " << this->connection->getSenderIp() << ":" << receivedSegment->sourcePort << std::endl;
                        
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
            connection->setDataStream(nullptr, 0); // Clear any previous data stream
            
            Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);

            // ACK request requires incrementing the received sequence number by 1
            Segment ackSegment = ack(&segment, connection->getCurrentSeqNum(), connection->getCurrentAckNum());
            
            for (int i = 0; i < connection->getMaxRetries(); ++i) {
                connection->send(this->connection->getSenderIp(), receivedSegment->sourcePort, &ackSegment, sizeof(ackSegment));
            }
            
            std::cout << Color::color("[i] [Closing]", Color::YELLOW) <<" [S=" << ackSegment.seqNum << "] " << "[A=" << ackSegment.ackNum << "] " <<  "Sending ACK request to " << this->connection->getSenderIp() << ":" << ackSegment.destPort << std::endl;

            cout << Color::color("Server Connection closed successfully.", Color::YELLOW) << endl;
            exit(0);
            
            break;
        }

        default:
            std::cout << Color::color("Unknown state!", Color::RED) << std::endl;
            break;
        }
    }
}
