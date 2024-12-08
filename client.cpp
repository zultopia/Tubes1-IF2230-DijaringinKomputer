#include "client.hpp"
#include "segment.hpp"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <thread>

Client::Client(std::string ip, int32_t port)
{
    this->connection = new TCPSocket(ip, port);
    this->sendingFile = false;
}

Client::~Client()
{
    connection->close();
    delete connection;
}

void Client::setSendingFile(bool isSendingFile){
    sendingFile = isSendingFile? 1: 0;
}

void Client::run()
{
    std::vector<uint8_t> fullBuffer; // Buffer of all the data received
    char buffer[2048]; // Buffer for receiving data
    std::cout << Color::color("Client is ready to initiate the handshake...", Color::MAGENTA) << std::endl;

    Segment *receivedSegment = nullptr;
    int32_t receivedBytes = 0;

    while (true)
    {
        switch (connection->getStatus())
        {
            case LISTEN:
            {
                connection->setDataStream(nullptr, 0);

                Segment segment = connection->generateSegmentsFromPayload(this->destPort);
                Segment syncSegment = syn(&segment, connection->getCurrentSeqNum());
                connection->send(this->destIP, this->destPort, &syncSegment, sizeof(syncSegment));

                std::cout << Color::color("[i] [Handshake]", Color::YELLOW) << "[S=" << syncSegment.seqNum << "] Sending SYN request to " << this->destIP << ":" << this->destPort << std::endl;


                connection->setStatus(SYN_SENT);

                break;
            }
            case SYN_SENT:
            {
                // Wait for ACK from the client
                receivedBytes = connection->recv(buffer, sizeof(buffer));
                if (receivedBytes > 0)
                {
                    receivedSegment = reinterpret_cast<Segment *>(buffer);
                    // Check if the destination port in segment is the same as this socket's port
                    if (receivedSegment->destPort == connection->getPort())
                    {
                        // Check if it's an SYN-ACK packet
                        if (receivedSegment->flags.syn && receivedSegment->flags.ack)
                        {
                            if (receivedSegment->ackNum == connection->getCurrentSeqNum() + 1)
                            {
                                connection->setCurrentSeqNum(receivedSegment->ackNum);
                                connection->setCurrentAckNum(receivedSegment->seqNum + 1);

                                std::cout << Color::color("[i] [Handshake]", Color::YELLOW) << "[S=" << receivedSegment->seqNum << "] " << "[A=" << receivedSegment->ackNum << "] " <<  "Received SYN-ACK request from " << this->destIP << ":" << this->destPort << std::endl;

                                // PINJAM STATE UNTUK RETRANSMIT
                                connection->setStatus(SYN_RECEIVED);
                                connection->setRetryAttempt(0);

                                if (receivedSegment->options.sendingFile){
                                    setSendingFile(true);
                                }

                                // // Prepare and send a ACK packet
                                // connection->setDataStream(nullptr);

                                // Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);
                                // Segment ackSegment = ack(&segment, receivedSegment->ackNum, receivedSegment->seqNum + 1);

                                // connection->send(connection->getSenderIp(), receivedSegment->sourcePort, &ackSegment, sizeof(ackSegment));
                                
                                // std::cout << "[Handshake] [S=" << ackSegment.seqNum << "] " << "[A=" << ackSegment.ackNum << "] " <<  "Sending ACK request to " << this->destIP << ":" << this->destPort << std::endl;
                                
                                // connection->setStatus(ESTABLISHED);
                                // connection->setRetryAttempt(0);

                                // std::cout << "Connection in Client established, ready to send/receive data" << std::endl;
                                break;
                            }
                            else
                            {
                                std::cout << Color::color("ACK number is incorrect.", Color::RED) << std::endl;
                            }
                        }
                    }
                }

                // Since we didn't receive the expected ACK, we will retransmit the SYN packet
                // Back to LISTEN state
                connection->setStatus(LISTEN);
                connection->setRetryAttempt(this->connection->getRetryAttempt() + 1);
                
                // // Wait for a short duration before retransmission
                // std::this_thread::sleep_for(std::chrono::milliseconds(connection->getWaitRetransmitTime()));

                // // Retransmit SYN packet
                // connection->setDataStream(nullptr);

                // Segment segment = connection->generateSegmentsFromPayload(this->destPort);
                // Segment syncSegment = syn(&segment, connection->getCurrentSeqNum());
                // connection->send(this->destIP, this->destPort, &syncSegment, sizeof(syncSegment));

                // std::cout << "[Handshake] [S=" << syncSegment.seqNum << "] Sending SYN request to " << this->destIP << ":" << this->destPort << std::endl;

                break;
            }
            case SYN_RECEIVED:
            {
                // Prepare and send a ACK packet
                connection->setDataStream(nullptr, 0);

                Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);
                Segment ackSegment = ack(&segment, connection->getCurrentSeqNum(), connection->getCurrentAckNum());

                connection->send(connection->getSenderIp(), receivedSegment->sourcePort, &ackSegment, sizeof(ackSegment));
                
                std::cout << Color::color("[i] [Handshake]", Color::YELLOW) << "[S=" << ackSegment.seqNum << "] " << "[A=" << ackSegment.ackNum << "] " <<  "Sending ACK request to " << this->destIP << ":" << this->destPort << std::endl;
                
                connection->setStatus(ESTABLISHED);

                std::cout << Color::color("Connection in Client established, ready to send/receive data", Color::YELLOW) << std::endl;

                break;
            }
            case ESTABLISHED:
            {
                std::string outputFileName = "";
                if (sendingFile)
                {
                    receivedBytes = connection->recv(buffer, sizeof(buffer));
                    if (!isValidChecksum(*receivedSegment)) {
                        std::cout << Color::color("[Warning] Received packet with invalid checksum.", Color::RED) << std::endl;
                        continue;
                    }
                    if (receivedBytes > 0)
                    {
                        receivedSegment = reinterpret_cast<Segment *>(buffer);
                        if (receivedSegment->flags.ack && !receivedSegment->flags.syn) {
                            std::string metadataStr(reinterpret_cast<char *>(receivedSegment->payload), MAX_PAYLOAD_SIZE);
                            
                            std::string fileName, fileSizeStr;
                            size_t fileNamePos = metadataStr.find("FILENAME:");
                            size_t fileSizePos = metadataStr.find("SIZE:");
                            if (fileNamePos != std::string::npos && fileSizePos != std::string::npos) {
                                fileNamePos += 9; // Length of "FILENAME:"
                                fileName = metadataStr.substr(fileNamePos, fileSizePos - fileNamePos - 1); // Extract filename
                                fileSizePos += 5; // Length of "SIZE:"
                                fileSizeStr = metadataStr.substr(fileSizePos, metadataStr.find(';', fileSizePos) - fileSizePos);
                                std::cout << Color::color("[i] [ESTABLISHED] Metadata received. File: ", Color::YELLOW) << fileName << ", Size: " << fileSizeStr << " bytes" << std::endl;
                            }
                            outputFileName = fileName;

                            Segment ackSegment = connection->generateSegmentsFromPayload(this->destPort);
                            ackSegment = ack(&ackSegment, receivedSegment->seqNum, receivedSegment->seqNum + metadataStr.size());
                            connection->send(connection->getSenderIp(), receivedSegment->sourcePort, &ackSegment, sizeof(ackSegment));
                            std::cout << Color::color("[+] [ESTABLISHED]", Color::GREEN) << "[S="<< ackSegment.seqNum <<"] [A="<< ackSegment.ackNum <<"] Sent ACK for metadata."<< std::endl;
                        }
                    }
                }

                auto startTime = std::chrono::steady_clock::now();

                size_t windowSize = MAX_PAYLOAD_SIZE;
                size_t LFR = connection->getCurrentAckNum() - 1;
                size_t LAF = LFR + windowSize;
                size_t seqNumAck = 0;
                
                std::vector<Segment> waitingSegments;
                std::vector<uint8_t> receivedData;
                uint32_t isChangeStatus = 0; // status change inside while loop
                
                while (true)
                {
                    receivedBytes = connection->recv(buffer, sizeof(buffer));
                    if (!isValidChecksum(*receivedSegment)) {
                        std::cout << Color::color("[Warning] Received packet with invalid checksum.", Color::RED) << std::endl;
                        continue;
                    }
                    if (receivedBytes > 0)
                    {
                        receivedSegment = reinterpret_cast<Segment *>(buffer);

                        if (!receivedSegment->flags.syn && receivedSegment->flags.ack)
                        {
                            if (LFR < receivedSegment->seqNum && receivedSegment->seqNum <= LAF) {
                                if (LFR == connection->getCurrentAckNum() - 1) {
                                    LFR = receivedSegment->seqNum - 1;
                                    LAF = LFR + windowSize;
                                    seqNumAck = receivedSegment->seqNum;
                                }

                                seqNumAck = receivedSegment->seqNum;
                                LFR = seqNumAck;
                                LAF = LFR + windowSize;
                                
                                size_t payloadSize = 0;
                                if (receivedSegment->flags.fin) {
                                    for (; payloadSize < MAX_PAYLOAD_SIZE; ++payloadSize) {
                                        if (receivedSegment->payload[payloadSize] == '\0') {
                                            break;
                                        }
                                    }
                                } else {
                                    payloadSize = MAX_PAYLOAD_SIZE;
                                }

                                // Append payload to received data
                                receivedData.insert(receivedData.end(),
                                    receivedSegment->payload,
                                    receivedSegment->payload + payloadSize);

                                std::string payloadStr(reinterpret_cast<char*>(receivedSegment->payload), payloadSize);
                                // fullBuffer.insert(fullBuffer.end(), receivedSegment->payload, receivedSegment->payload + payloadSize);

                                if (!sendingFile){
                                    std::cout << Color::color("[i] [Established]", Color::GREEN) <<" [S=" << receivedSegment->seqNum << "] ACKed with payload : " << payloadStr << std::endl;
                                } else {
                                    std::cout << Color::color("[i] [Established]", Color::GREEN) <<" [S=" << receivedSegment->seqNum << "] ACKed" << std::endl;

                                }

                                Segment segment = connection->generateSegmentsFromPayload(this->destPort);
                                Segment ackSegment = ack(&segment, receivedSegment->seqNum, receivedSegment->seqNum + payloadSize);

                                connection->send(connection->getSenderIp(), receivedSegment->sourcePort, &ackSegment, sizeof(ackSegment));
                                std::cout << Color::color("[i] [Established]", Color::YELLOW) <<" [A=" << ackSegment.ackNum << "] Sent" << std::endl;

                                if (receivedSegment->flags.fin) {
                                    // std::string str(fullBuffer.begin(), fullBuffer.end());
                                    // std::cout << "[i] fullBuffer content as string: " << str << std::endl;
                                    if (sendingFile) {
                                        std::ofstream outputFile(outputFileName, std::ios::binary);
                                        if (outputFile.is_open()) {
                                            outputFile.write(reinterpret_cast<const char *>(receivedData.data()), receivedData.size());
                                            outputFile.close();
                                            std::cout << Color::color("[+] File saved as: ", Color::GREEN) << outputFileName << std::endl;
                                        } else {
                                            std::cerr << Color::color("[-] Failed to save file.", Color::RED) << std::endl;
                                        }
                                    }else {
                                        // Print received text
                                        std::string receivedPayload(receivedData.begin(), receivedData.end());
                                        std::cout << Color::color("[i] [Established]", Color::GREEN)
                                                << "Received message: " << receivedPayload << std::endl;
                                    }
                                    connection->setStatus(FIN_WAIT_2);
                                    connection->setRetryAttempt(0);
                                    break;
                                }
                            }
                        }
                    }

                    auto elapsedTime = std::chrono::steady_clock::now() - startTime;
                    if (std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count() >= 3)
                    {
                        // std::string receivedPayload(fullBuffer.begin(), fullBuffer.end());

                        // if (receivedPayload.length() == 0)
                        if (receivedData.empty())
                        {
                            cout << Color::color("It seems like the server is not established yet", Color::RED) << endl;
                            connection->setStatus(SYN_RECEIVED);
                            connection->setRetryAttempt(connection->getRetryAttempt() + 1);

                            isChangeStatus = 1;

                            break;
                        } 
                    }
                }

                break;
            }
            case FIN_WAIT_1:
            {
                break;
            }
            case FIN_WAIT_2:
            {
                cout << Color::color("[i] Client is in FIN_WAIT_2 state.", Color::YELLOW) << endl;
                // Wait for FIN from the client
                receivedBytes = connection->recv(buffer, sizeof(buffer));
                receivedSegment = reinterpret_cast<Segment *>(buffer);
                while (receivedBytes > 0 && !(!receivedSegment->flags.syn && !receivedSegment->flags.ack && receivedSegment->flags.fin)) {
                    receivedBytes = connection->recv(buffer, sizeof(buffer), 10);
                    receivedSegment = reinterpret_cast<Segment *>(buffer);
                }                

                if (receivedBytes > 0)
                {
                    receivedSegment = reinterpret_cast<Segment *>(buffer);

                    // Validate destination port
                    if (receivedSegment->destPort == connection->getPort())
                    {
                        // Check if it's an FIN packet
                        if (!receivedSegment->flags.syn && !receivedSegment->flags.ack && receivedSegment->flags.fin)
                        {
                            connection->setCurrentSeqNum(receivedSegment->ackNum);
                            connection->setCurrentAckNum(receivedSegment->seqNum + 1);

                            std::cout << Color::color("[+] [Closing]", Color::GREEN) << " [S=" << receivedSegment->seqNum << "] Receiving FIN request from " << this->connection->getSenderIp() << ":" << receivedSegment->sourcePort << std::endl;

                            connection->setStatus(CLOSE_WAIT);
                            connection->setRetryAttempt(0);

                            break;
                        }
                    }
                }

                connection->setRetryAttempt(connection->getRetryAttempt() + 1);

                if (connection->getRetryAttempt() >= connection->getMaxRetries())
                {
                    std::cout << Color::color("Max retries reached.", Color::RED) << std::endl;
                    exit(1);
                }
                
                break;
            }
            case CLOSE_WAIT:
            {
                // prepare and send FIN-ACK packet
                connection->setDataStream(nullptr, 0);

                Segment segment = connection->generateSegmentsFromPayload(this->destPort);

                Segment finAckSegment = finAck(&segment, connection->getCurrentSeqNum(), connection->getCurrentAckNum());

                connection->send(this->destIP, this->destPort, &finAckSegment, sizeof(finAckSegment));

                std::cout << Color::color("[i] [Closing]", Color::YELLOW) <<" [S=" << finAckSegment.seqNum << "] " << "[A=" << finAckSegment.ackNum << "] " <<  "Sending FIN-ACK request to " << this->destIP << ":" << this->destPort << std::endl;

                connection->setStatus(LAST_ACK);
                connection->setRetryAttempt(0);
                
                break;
            }
            case CLOSING:
            {
                // /* code */
                // cout << "Client is in CLOSING state." << endl;

                // // Wait for the ACK of the FIN
                // receivedBytes = connection->recv(buffer, sizeof(buffer));
                // if (receivedBytes > 0)
                // {
                //     receivedSegment = reinterpret_cast<Segment *>(buffer);

                //     // If an ACK is received, transition to TIME_WAIT
                //     if (receivedSegment->flags.ack)
                //     {
                //         cout << "[ACK Received] Transitioning to TIME_WAIT state." << endl;
                //         this->state = TIME_WAIT;
                //     }
                // }
                break;
            }
            case LAST_ACK:
            {
                // Wait for LAST ACK from the client
                receivedBytes = connection->recv(buffer, sizeof(buffer));
                if (receivedBytes > 0)
                {
                    receivedSegment = reinterpret_cast<Segment *>(buffer);

                    // Validate destination port
                    if (receivedSegment->destPort == connection->getPort())
                    {
                        // Check if it's an FIN-ACK packet
                        if (!receivedSegment->flags.syn && receivedSegment->flags.ack && !receivedSegment->flags.fin)
                        {
                            connection->setCurrentSeqNum(receivedSegment->ackNum);
                            connection->setCurrentAckNum(receivedSegment->seqNum + 1);

                            std::cout << Color::color("[+] [Closing]", Color::GREEN) << " [S=" << receivedSegment->seqNum << "] Receiving ACK request from " << this->connection->getSenderIp() << ":" << receivedSegment->sourcePort << std::endl;
                            
                            cout << Color::color("Client Connection closed successfully.", Color::YELLOW) << endl;
                            exit(0);
                        }
                    }
                }

                connection->setStatus(CLOSE_WAIT);
                connection->setRetryAttempt(connection->getRetryAttempt() + 1);
                
                break;
            }
            default:
                break;
        }
    }
}

void Client::setDestination()
{
    std::cout << Color::color("[?] Enter the destination IP: ", Color::CYAN);
    std::cin >> destIP;
    // destIP = "127.0.0.1";
    std::cout << Color::color("[?] Enter the destination port: ", Color::CYAN);
    std::cin >> destPort;
    // destPort = 8031;
}
