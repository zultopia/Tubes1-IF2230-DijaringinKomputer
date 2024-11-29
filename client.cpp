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
                            std::cout << "Client received: SYN+ACK packet from Server." << std::endl;

                            if (receivedSegment->ackNum == connection->getCurrentSeqNum() + 1)
                            {
                                std::cout << "ACK number is correct." << std::endl;

                                // Prepare and send a ACK packet
                                connection->setDataStream(nullptr);

                                Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);
                                Segment ackSegment = ack(&segment, receivedSegment->ackNum, receivedSegment->seqNum + 1);

                                connection->send(connection->getSenderIp(), receivedSegment->sourcePort, &ackSegment, sizeof(ackSegment));
                                std::cout << "Client sent ACK packet to Server." << std::endl;

                                connection->setStatus(ESTABLISHED);

                                break;
                            }
                            else
                            {
                                std::cout << "ACK number is incorrect." << std::endl;
                                break;
                            }
                        }
                    }
                }
                connection->setDataStream(nullptr);

                Segment segment = connection->generateSegmentsFromPayload(this->destPort);
                Segment syncSegment = syn(&segment, connection->getCurrentSeqNum());
                connection->send(this->destIP, this->destPort, &syncSegment, sizeof(syncSegment));
                connection->setStatus(SYN_SENT);

                break;
            }
            case SYN_RECEIVED:
            {
                /* code */
                break;
            }
            case ESTABLISHED:
            {
                std::cout << "Connection in Client established, ready to send/receive data" << std::endl;
                /* code */
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