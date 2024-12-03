#include "client.hpp"
#include <iostream>

int main() {
    std::string clientHost;
    int clientPort;

    // Create and start the client
    Client client("127.0.0.1", 8032);
    client.setDestination();
    client.run();

    return 0;
}
