#include "client.hpp"
#include "segment.hpp"
#include <iostream>
#include <unistd.h>
#include <thread>

Client::Client(std::string ip, int32_t port)
{
    this->port = port;
    this->connection = new TCPSocket(ip, port);
}

Client::~Client()
{
    connection->close();
    delete connection;
}

void Client::run()
{
    std::string message = "Hello, Server!";
    char buffer[1024]; // Buffer for receiving data
    std::cout << "Server is waiting for incoming data..." << std::endl;

    Segment *receivedSegment = nullptr; // Declare outside for reuse
    int32_t receivedBytes = 0;

    while (true)
    {
        switch (connection->getStatus())
        {
            case LISTEN:
            {
                connection->setDataStream(nullptr);
                // connection->setDataStream(reinterpret_cast<uint8_t*>(message.data()));
                Segment segment = connection->generateSegmentsFromPayload(8031);
                Segment syncSegment = syn(&segment, connection->getCurrentSeqNum());
                connection->send("127.0.0.1", 8031, &syncSegment, sizeof(syncSegment));
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

                    std::cout << "Received " << receivedBytes << " bytes from Server." << std::endl;

                    // Validate destination port
                    if (receivedSegment->destPort == connection->getPort())
                    {
                        // Check if it's an SYN+ACK packet
                        if (receivedSegment->flags.syn && receivedSegment->flags.ack)
                        {
                            std::cout << "Server received: SYN+ACK packet from Client." << std::endl;
                            // Prepare and send a SYN-ACK packet
                            connection->setDataStream(nullptr); // Clear any previous data stream
                            Segment segment = connection->generateSegmentsFromPayload(receivedSegment->sourcePort);

                            // ACK requires incrementing the received sequence number by 1
                            Segment ackSegment = ack(&segment, connection->getCurrentSeqNum(), receivedSegment->seqNum + 1);

                            connection->send("127.0.0.1", receivedSegment->sourcePort, &ackSegment, sizeof(ackSegment));
                            std::cout << "Server sent SYN-ACK packet to Client." << std::endl;

                            connection->setStatus(ESTABLISHED);

                            std::cout << "Connection ESTABLISHED." << std::endl;
                        }
                    }
                }
                connection->setDataStream(nullptr);
                // connection->setDataStream(reinterpret_cast<uint8_t*>(message.data()));
                Segment segment = connection->generateSegmentsFromPayload(8031);
                Segment syncSegment = syn(&segment, connection->getCurrentSeqNum());
                connection->send("127.0.0.1", 8031, &syncSegment, sizeof(syncSegment));
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
        // // Send a message to the server
        // connection->send("127.0.0.1", 8031, message.data(), message.size());
        // std::cout << "Client sent: " << message << std::endl;

        // // Receive the server's response
        // int32_t receivedBytes = connection->recv(buffer, sizeof(buffer));
        // if (receivedBytes > 0)
        // {
        //     // Null-terminate and print the response
        //     buffer[receivedBytes] = '\0';
        //     std::cout << "Client received: " << buffer << std::endl;
        // }

        // // Wait for 1 second before sending the next message
        // std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
