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
    
    Server server("127.0.0.1", 8031);
    std::thread serverThread([&server]() {
        server.run();
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));

    Client client("127.0.0.1", 8032);
    client.setDestination();
    client.run();

    if (serverThread.joinable()) {
        serverThread.join();
    }

    return 0;
}