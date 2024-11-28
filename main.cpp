#include "server.hpp"
#include "client.hpp"
#include <thread>

int main()
{
    // Create and start the server
    Server server("127.0.0.1", 8031);
    std::thread serverThread([&server]() {
        server.run();
    });

    // Allow the server to initialize (optional delay)
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Create and start the client
    Client client("127.0.0.1", 8032); // Client uses a different port for its socket
    client.run();

    // Join the server thread
    serverThread.join();

    return 0;
}
