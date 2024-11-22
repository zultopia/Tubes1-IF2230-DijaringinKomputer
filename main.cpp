#include "server.hpp"
#include "client.hpp"

int main()
{
    Server server("127.0.0.1", 8080);
    server.run(); 
    return 0;

    // Client client("127.0.0.1", 8080);
    // client.run(); 
    // return 0;
}