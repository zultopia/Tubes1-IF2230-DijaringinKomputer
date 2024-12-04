#include <iostream>
#include <string>
#include "server.hpp"
#include "client.hpp"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "[!] Incorrect number of arguments. Usage: [executable] [host] [port]" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    int port = std::stoi(argv[2]);

    std::cout << "[i] Node started at " << host << ":" << port << std::endl;

    int mode;
    std::cout << "[?] Please choose the operating mode" << std::endl;
    std::cout << "[?] 1. Sender" << std::endl;
    std::cout << "[?] 2. Receiver" << std::endl;
    std::cout << "[?] Input: ";
    std::cin >> mode;

    if (mode == 1) {
        Server server(host, port);
        std::cout << "[+] Node is now a sender" << std::endl;

        int sendMode;
        std::cout << "[?] Please choose the sending mode" << std::endl;
        std::cout << "[?] 1. User input" << std::endl;
        std::cout << "[?] 2. File input" << std::endl;
        std::cout << "[?] Input: ";
        std::cin >> sendMode;

        if (sendMode == 1) {
            std::cin.ignore();
            std::string userInput;
            std::cout << "[?] Input mode chosen, please enter your input: ";
            std::getline(std::cin, userInput);
            server.setData(userInput);

            std::cout << "[+] User input has been successfully received." << std::endl;
            std::cout << "[i] Listening to the broadcast port for clients." << std::endl;

            server.run();
        } else if (sendMode == 2) {
            std::cin.ignore();
            std::string fileName;
            std::cout << "[?] Please enter the file path: ";
            std::getline(std::cin, fileName);

            std::cout << "[+] File " << fileName << " has been successfully read." << std::endl;
            std::cout << "[i] Listening to the broadcast port for clients." << std::endl;

        } else {
            std::cerr << "[!] Invalid choice. Exiting..." << std::endl;
        }
    } else if (mode == 2) {
        Client client(host, port);
        client.setDestination();
        client.run();

        std::cout << "[+] Node is now a receiver" << std::endl;
    } else {
        std::cerr << "[!] Invalid choice. Exiting..." << std::endl;
    }

    return 0;
}
