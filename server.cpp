#include "server.hpp"
#include <iostream>
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
            /* code */
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

        }

        case ESTABLISHED:
        {
            // exit(0);
            break;
        }

        case FIN_WAIT_1:
        case FIN_WAIT_2:
        case CLOSE_WAIT:
        case CLOSING:
        case LAST_ACK:
        {
            std::cout << "Server is in connection closing state." << std::endl;
            exit(0);
            break;
        }

        default:
            std::cout << "Unknown state!" << std::endl;
            break;
        }
    }
}
