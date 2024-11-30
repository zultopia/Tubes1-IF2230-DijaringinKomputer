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
            std::cout << "Server is in SYN_SENT state." << std::endl;

            // Wait for SYN-ACK from the client
            receivedBytes = connection->recv(buffer, sizeof(buffer));
            if (receivedBytes > 0)
            {
                receivedSegment = reinterpret_cast<Segment *>(buffer);

                if (receivedSegment->flags.syn && receivedSegment->flags.ack)
                {
                    // This is a SYN-ACK from the client
                    std::cout << "[SYN-ACK Received] Transitioning to ESTABLISHED state." << std::endl;

                    // Send an ACK to complete the handshake
                    Segment ackSegment;
                    ackSegment.seqNum = connection->getCurrentSeqNum();
                    ackSegment.ackNum = connection->getCurrentAckNum();
                    ackSegment.flags.ack = true;

                    connection->send(connection->getSenderIp(), this->destPort, &ackSegment, sizeof(ackSegment));

                    // Transition to ESTABLISHED state after sending the ACK
                    this->state = ESTABLISHED;
                }
            }
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
            // Vector to store received payloads
            std::vector<std::string> messageVector;

            cout << "Server is in ESTABLISHED state." << endl;
            int retryCount = 0;               // Initialize retry counter
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

                            // Store the payload in the vector to maintain the order
                            messageVector.push_back(receivedSegment->payload);

                            // Acknowledge receipt of the payload
                            Segment ackSegment = ack(receivedSegment, connection->getCurrentAckNum(), connection->getCurrentSeqNum());
                            connection->send(connection->getSenderIp(), this->destPort, &ackSegment, sizeof(ackSegment));

                            messageReceived = true;  // Message received successfully
                            break;  // Exit the retry loop
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

                    // Resend the ACK
                    Segment ackSegment = ack(receivedSegment, connection->getCurrentAckNum(), connection->getCurrentSeqNum());
                    connection->send(connection->getSenderIp(), this->destPort, &ackSegment, sizeof(ackSegment));

                    // Wait before retrying
                    std::this_thread::sleep_for(std::chrono::milliseconds(connection->getWaitRetransmitTime()));
                }
            }

            if (!messageReceived)
            {
                std::cerr << "[Error] Failed to receive any messages after " << MAX_RETRIES << " retries. Connection may be lost." << std::endl;
                exit(1);
            }

            // Print all received messages in order (optional, for debugging)
            std::cout << "[Server] All received messages in order:" << std::endl;
            for (const auto &msg : messageVector)
            {
                std::cout << msg << std::endl;
            }

            break;
        }

        case FIN_WAIT_1:
        {
            std::cout << "Server is in FIN_WAIT_1 state." << std::endl;

            // Wait for an ACK or FIN from the client
            receivedBytes = connection->recv(buffer, sizeof(buffer));
            if (receivedBytes > 0)
            {
                receivedSegment = reinterpret_cast<Segment *>(buffer);

                // If the client sends an ACK for the FIN, transition to FIN_WAIT_2
                if (receivedSegment->flags.ack)
                {
                    std::cout << "[ACK Received] Transitioning to FIN_WAIT_2 state." << std::endl;
                    this->state = FIN_WAIT_2;
                }
                else if (receivedSegment->flags.fin)
                {
                    // If the client sends a FIN, send an ACK and transition to CLOSING state
                    std::cout << "[FIN Received] Transitioning to CLOSING state." << std::endl;
                    Segment ackSegment;
                    ackSegment.seqNum = connection->getCurrentSeqNum();
                    ackSegment.ackNum = connection->getCurrentAckNum();
                    ackSegment.flags.ack = true;

                    connection->send(connection->getSenderIp(), this->destPort, &ackSegment, sizeof(ackSegment));
                    this->state = CLOSING;
                }
            }
            break;
        }

        case FIN_WAIT_2:
        {
            std::cout << "Server is in FIN_WAIT_2 state." << std::endl;

            // Wait for a FIN from the client
            receivedBytes = connection->recv(buffer, sizeof(buffer));
            if (receivedBytes > 0)
            {
                receivedSegment = reinterpret_cast<Segment *>(buffer);

                if (receivedSegment->flags.fin)
                {
                    std::cout << "[FIN Received] Transitioning to TIME_WAIT state." << std::endl;
                    // Transition to TIME_WAIT (not shown but logically follows)
                    this->state = TIME_WAIT;

                    // Send an ACK to acknowledge the FIN
                    Segment ackSegment;
                    ackSegment.seqNum = connection->getCurrentSeqNum();
                    ackSegment.ackNum = connection->getCurrentAckNum();
                    ackSegment.flags.ack = true;

                    connection->send(connection->getSenderIp(), this->destPort, &ackSegment, sizeof(ackSegment));
                }
            }
            break;
        }

        case CLOSE_WAIT:
        {
            std::cout << "Server is in CLOSE_WAIT state." << std::endl;

            // Wait for an ACK from the client to complete the connection termination
            receivedBytes = connection->recv(buffer, sizeof(buffer));
            if (receivedBytes > 0)
            {
                receivedSegment = reinterpret_cast<Segment *>(buffer);

                // Send an ACK to acknowledge the FIN from the client
                if (receivedSegment->flags.fin)
                {
                    std::cout << "[FIN Received] Sending ACK and transitioning to LAST_ACK state." << std::endl;
                    Segment ackSegment;
                    ackSegment.seqNum = connection->getCurrentSeqNum();
                    ackSegment.ackNum = connection->getCurrentAckNum();
                    ackSegment.flags.ack = true;

                    connection->send(connection->getSenderIp(), this->destPort, &ackSegment, sizeof(ackSegment));

                    // Transition to LAST_ACK
                    this->state = LAST_ACK;
                }
            }
            break;
        }

        case CLOSING:
        {
            std::cout << "Server is in CLOSING state." << std::endl;

            // Wait for the final ACK from the client to complete termination
            receivedBytes = connection->recv(buffer, sizeof(buffer));
            if (receivedBytes > 0)
            {
                receivedSegment = reinterpret_cast<Segment *>(buffer);

                // If an ACK is received, transition to TIME_WAIT
                if (receivedSegment->flags.ack)
                {
                    std::cout << "[ACK Received] Transitioning to TIME_WAIT state." << std::endl;
                    this->state = TIME_WAIT;
                }
            }
            break;
        }

        case LAST_ACK:
        {
            std::cout << "Server is in LAST_ACK state." << std::endl;

            // Wait for the final ACK from the client after sending the FIN
            receivedBytes = connection->recv(buffer, sizeof(buffer));
            if (receivedBytes > 0)
            {
                receivedSegment = reinterpret_cast<Segment *>(buffer);

                // If an ACK is received, we can safely close the connection
                if (receivedSegment->flags.ack)
                {
                    std::cout << "[ACK Received] Closing the connection." << std::endl;
                    // Close connection or perform any final cleanup
                    connection->close();
                    exit(0); // Exit since the connection is closed
                }
            }
            break;
        }

        default:
            std::cout << "Unknown state!" << std::endl;
            break;
        }
    }
}
