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
                /* code */
                break;
            }
            case ESTABLISHED:
            {
                cout << "Client is in ESTABLISHED state." << endl;
                int retryCount = 0;                // Initialize retry counter
                constexpr int MAX_RETRIES = 5;    // Define a maximum number of retries
                bool messageReceived = false;     // Flag to check if a real message is received

                while (retryCount < MAX_RETRIES && !messageReceived)
                {
                    // Wait for incoming REAL MESSAGE packets
                    receivedBytes = connection->recv(buffer, sizeof(buffer));
                    if (receivedBytes > 0)
                    {
                        receivedSegment = reinterpret_cast<Segment *>(buffer);

                        // Validate the received segment
                        if (connection->getSenderIp() == this->destIP && receivedSegment->destPort == connection->getPort())
                        {
                            if (!receivedSegment->flags.syn && receivedSegment->flags.ack) // Real data packet (not part of handshake)
                            {
                                std::cout << "[Data] Received message from " << this->destIP << ":" << this->destPort << " with payload: "
                                        << receivedSegment->payload << std::endl;

                                // sending ACK

                                messageReceived = true; // Message received successfully
                                break;                  // Exit the retry loop
                            }
                            else
                            {
                                std::cout << "[Warning] Received unexpected packet during ESTABLISHED state." << std::endl;
                            }
                        }
                    }
                    else
                    {
                        // No packet received; handle retransmission
                        retryCount++;

                        std::cout << "[Retry] No data received. Resending ACK packet (" << retryCount << "/" << MAX_RETRIES << ")." << std::endl;

                        // Prepare and resend the last ACK packet
                        connection->setDataStream(nullptr);
                        Segment segment = connection->generateSegmentsFromPayload(this->destPort);
                        Segment ackSegment = ack(&segment, connection->getCurrentAckNum(), connection->getCurrentSeqNum());

                        connection->send(connection->getSenderIp(), this->destPort, &ackSegment, sizeof(ackSegment));

                        // Wait for a short duration before the next retry
                        std::this_thread::sleep_for(std::chrono::milliseconds(connection->getWaitRetransmitTime()));
                    }
                }

                if (!messageReceived)
                {
                    std::cerr << "[Error] Failed to receive any messages after " << MAX_RETRIES << " retries. Connection may be lost." << std::endl;
                    // Optionally, reset connection or handle failure here
                    exit(1);
                }

                break;
            }
            case FIN_WAIT_1:
            {
                /* code */
                break;
            }
            case FIN_WAIT_2:
            {
                /* code */
                break;
            }
            case CLOSE_WAIT:
            {
                /* code */
                break;
            }
            case CLOSING:
            {
                /* code */
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
    std::cin >> destIP;
    std::cout << "Enter the destination port: ";
    std::cin >> destPort;
}