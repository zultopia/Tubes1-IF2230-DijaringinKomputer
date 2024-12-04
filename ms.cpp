#include "server.hpp"
#include <thread>
#include <iostream>

int main() {
    std::string serverHost;
    int serverPort;

    // Create and start the server
    Server server("127.0.0.1", 8031);
    server.setData("Tes");
    server.run();

    return 0;
}
