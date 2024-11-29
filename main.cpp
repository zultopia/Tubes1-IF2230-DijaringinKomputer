#include "server.hpp"
#include "client.hpp"
#include <thread>
#include <iostream>

int main()
{
    std::string serverHost;
    int serverPort;
    std::string clientHost;
    int clientPort;

    // Create and start the server
    // ask to input the host and port
    std::cout << "Enter the server host: ";
    std::cin >> serverHost;
    std::cout << "Enter the server port: ";
    std::cin >> serverPort;
    
    Server server(serverHost, serverPort);
    // Server server("127.0.0.1", 8031);
    std::thread serverThread([&server]() {
        server.run();
    });

    // Allow the server to initialize (optional delay)
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Create and start the client
    // ask to input the host and port
    std::cout << "Enter the client host: ";
    std::cin >> clientHost;
    std::cout << "Enter the client port: ";
    std::cin >> clientPort;

    Client client(clientHost, clientPort);
    client.setDestination();
    // Client client("127.0.0.1", 8032);
    client.run();

    // Join the server thread
    serverThread.join();

    return 0;
}
