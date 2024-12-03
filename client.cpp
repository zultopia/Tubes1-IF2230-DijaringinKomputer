#include "client.hpp"
#include "segment.hpp"
#include <iostream>
#include <unistd.h>
#include <thread>

Client::Client(std::string ip, int32_t port)
{
    this->connection = new TCPSocket(ip, port);
}

Client::~Client()
{
    connection->close();
    delete connection;
}

void Client::run()
{
    std::vector<uint8_t> fullBuffer; // Buffer of all the data received
    char buffer[1024]; // Buffer for receiving data
    std::cout << "Client is ready to initiate the handshake..." << std::endl;

    Segment *receivedSegment = nullptr;
    int32_t receivedBytes = 0;

    while (true)
    {
        switch (connection->getStatus())
        {
            case LISTEN:
            {
                connection->setDataStream(nullptr);

                Segment segment = connection->generateSegmentsFromPayload(this->destPort);
                Segment syncSegment = syn(&segment, connection->getCurrentSeqNum());
                connection->send(this->destIP, this->destPort, &syncSegment, sizeof(syncSegment));

                std::cout << "[Handshake] [S=" << syncSegment.seqNum << "] Sending SYN request to " << this->destIP << ":" << this->destPort << std::endl;


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
                    if (connection->getSenderIp() == this->destIP && receivedSegment->destPort == connection->getPort())
                    {
                        // Check if it's an SYN+ACK packet
                        if (receivedSegment->flags.syn && receivedSegment->flags.ack)
                        {
                            if (receivedSegment->ackNum == connection->getCurrentSeqNum() + 1)
                            {
                                std::cout << "[Handshake] [S=" << receivedSegment->seqNum << "] " << "[A=" << receivedSegment->ackNum << "] " <<  "Received SYN-ACK request from " << this->destIP << ":" << this->destPort << std::endl;

                                // Prepare and send a ACK packet
                                connection->setDataStream(nullptr);

                                Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);
                                Segment ackSegment = ack(&segment, receivedSegment->ackNum, receivedSegment->seqNum + 1);

                                connection->send(connection->getSenderIp(), receivedSegment->sourcePort, &ackSegment, sizeof(ackSegment));
                                
                                std::cout << "[Handshake] [S=" << ackSegment.seqNum << "] " << "[A=" << ackSegment.ackNum << "] " <<  "Sending ACK request to " << this->destIP << ":" << this->destPort << std::endl;
                                
                                connection->setStatus(ESTABLISHED);
                                std::cout << "Connection in Client established, ready to send/receive data" << std::endl;

                                break;
                            }
                            else
                            {
                                std::cout << "ACK number is incorrect." << std::endl;
                            }
                        }
                    }
                }
                // Wait for a short duration before retransmission
                std::this_thread::sleep_for(std::chrono::milliseconds(connection->getWaitRetransmitTime()));

                // Retransmit SYN packet
                connection->setDataStream(nullptr);

                Segment segment = connection->generateSegmentsFromPayload(this->destPort);
                Segment syncSegment = syn(&segment, connection->getCurrentSeqNum());
                connection->send(this->destIP, this->destPort, &syncSegment, sizeof(syncSegment));

                std::cout << "[Handshake] [S=" << syncSegment.seqNum << "] Sending SYN request to " << this->destIP << ":" << this->destPort << std::endl;

                break;
            }
            case SYN_RECEIVED:
            {
                break;
            }
            case ESTABLISHED:
            {
                cout << "Client is in ESTABLISHED state." << endl;

                auto startTime = std::chrono::steady_clock::now();  // Record start time
                
                while (true)
                {
                    receivedBytes = connection->recv(buffer, sizeof(buffer));
                    if (receivedBytes > 0)
                    {
                        receivedSegment = reinterpret_cast<Segment *>(buffer);

                        if (!receivedSegment->flags.syn && receivedSegment->flags.ack)
                        {
                            std::string payloadStr(reinterpret_cast<char*>(receivedSegment->payload), MAX_PAYLOAD_SIZE);
                            fullBuffer.insert(fullBuffer.end(), receivedSegment->payload, receivedSegment->payload + MAX_PAYLOAD_SIZE);

                            std::cout << "[Established] [S=" << receivedSegment->seqNum << "] ACKed with payload: " << payloadStr << std::endl;

                            Segment segment = connection->generateSegmentsFromPayload(this->destPort);
                            Segment ackSegment = ack(&segment, 0, receivedSegment->seqNum);

                            connection->send(connection->getSenderIp(), receivedSegment->sourcePort, &ackSegment, sizeof(ackSegment));
                            std::cout << "[Established] [A=" << receivedSegment->seqNum << "] Sent" << std::endl;
                        }
                        else
                        {
                            std::cout << "[Warning] Received unexpected packet during ESTABLISHED state." << std::endl;
                        }
                    }

                    auto elapsedTime = std::chrono::steady_clock::now() - startTime;
                    if (std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count() >= 3)
                    {
                        std::cout << "[Timeout] No message received within 5 seconds. Exiting..." << std::endl;
                        break;
                    }
                }

                std::string str(fullBuffer.begin(), fullBuffer.end());
                std::cout << "fullBuffer content as string: " << str << std::endl;
                exit(1);

                break;
            }
            case FIN_WAIT_1:
            {
                // /* code */
                // cout << "Client is in FIN_WAIT_1 state." << endl;

                // // Wait for an ACK or FIN from the server
                // receivedBytes = connection->recv(buffer, sizeof(buffer));
                // if (receivedBytes > 0)
                // {
                //     receivedSegment = reinterpret_cast<Segment *>(buffer);

                //     // If the segment is an ACK, move to FIN_WAIT_2
                //     if (receivedSegment->flags.ack)
                //     {
                //         cout << "[ACK Received] Transitioning to FIN_WAIT_2 state." << endl;
                //         this->state = FIN_WAIT_2;
                //     }
                //     else if (receivedSegment->flags.fin)
                //     {
                //         // If the segment is a FIN, send an ACK and transition to CLOSING state
                //         cout << "[FIN Received] Transitioning to CLOSING state." << endl;
                //         this->state = CLOSING;
                //         // Send an ACK to acknowledge the FIN
                //         Segment ackSegment;
                //         ackSegment.seqNum = connection->getCurrentSeqNum();
                //         ackSegment.ackNum = connection->getCurrentAckNum();
                //         ackSegment.flags.ack = true;
                //         connection->send(connection->getSenderIp(), this->destPort, &ackSegment, sizeof(ackSegment));
                //     }
                // }
                // break;
            }
            case FIN_WAIT_2:
            {
                // /* code */
                // cout << "Client is in FIN_WAIT_2 state." << endl;

                // // Wait for a FIN from the server
                // receivedBytes = connection->recv(buffer, sizeof(buffer));
                // if (receivedBytes > 0)
                // {
                //     receivedSegment = reinterpret_cast<Segment *>(buffer);

                //     if (receivedSegment->flags.fin)
                //     {
                //         cout << "[FIN Received] Transitioning to TIME_WAIT state." << endl;
                //         // Transition to TIME_WAIT (not shown in original, but logically follows)
                //         this->state = TIME_WAIT;

                //         // Send an ACK to acknowledge the FIN
                //         Segment ackSegment;
                //         ackSegment.seqNum = connection->getCurrentSeqNum();
                //         ackSegment.ackNum = connection->getCurrentAckNum();
                //         ackSegment.flags.ack = true;
                //         connection->send(connection->getSenderIp(), this->destPort, &ackSegment, sizeof(ackSegment));
                //     }
                // }
                break;
            }
            case CLOSE_WAIT:
            {
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
                break;
            }
            default:
                break;
        }
    }
}

void Client::setDestination()
{
    // Ask the user for the destination IP and port
    std::cout << "Enter the destination IP: ";
    // std::cin >> destIP;
    destIP = "127.0.0.1";
    std::cout << "Enter the destination port: ";
    // std::cin >> destPort;
    destPort = 8031;
}