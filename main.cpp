#include <iostream>
#include <string>
#include "color.hpp"
#include "server.hpp"
#include "client.hpp"

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cerr << Color::color("[!] Incorrect number of arguments. Usage: [executable] [host] [port]", Color::RED) << std::endl;
        return 1;
    }

    std::string host = argv[1];
    int port = std::stoi(argv[2]);

    std::cout << Color::color("[i] Node started at ", Color::YELLOW) << host << ":" << port << std::endl;

    int mode;
    std::cout << Color::color("[?] Please choose the operating mode", Color::CYAN)<< std::endl;
    std::cout << Color::color("[?] 1. Sender", Color::CYAN)<< std::endl;
    std::cout << Color::color("[?] 2. Receiver", Color::CYAN)<< std::endl;
    std::cout << Color::color("[?] Input: ", Color::CYAN);
    std::cin >> mode;

    if (mode == 1) {
        Server server(host, port);
        std::cout << Color::color("[+] Node is now a sender", Color::GREEN) << std::endl;

        int sendMode;
        std::cout << Color::color("[?] Please choose the sending mode", Color::CYAN) << std::endl;
        std::cout << Color::color("[?] 1. Text Input", Color::CYAN) << std::endl;
        std::cout << Color::color("[?] 2. File Input", Color::CYAN) << std::endl;
        std::cout << Color::color("[?] Input: ", Color::CYAN);
        std::cin >> sendMode;

        if (sendMode == 1) {
            std::cin.ignore();
            std::string userInput;
            std::cout << Color::color("[?] Input mode chosen, please enter your input: ", Color::CYAN);
            std::getline(std::cin, userInput);
            server.setData(userInput);

            std::cout << Color::color("[+] User input has been successfully received.", Color::GREEN) << std::endl;
            std::cout << Color::color("[i] Listening to the broadcast port for clients.", Color::YELLOW) << std::endl;

            server.run();
        } else if (sendMode == 2) {
           std::cin.ignore();
            std::string fileName;
            std::cout << Color::color("[?] Please enter the file path: ", Color::CYAN);
            std::getline(std::cin, fileName);

            std::cout << Color::color("[+] File " + fileName + " has been successfully read.", Color::GREEN) << std::endl;
            std::cout << Color::color("[i] Listening to the broadcast port for clients.", Color::YELLOW) << std::endl;

        } else {
            std::cerr << Color::color("[!] Invalid choice. Exiting...", Color::RED) << std::endl;
        }
    } else if (mode == 2) {
        Client client(host, port);
        client.setDestination();
        client.run();

        std::cout << Color::color("[+] Node is now a receiver", Color::GREEN) << std::endl;
    } else {
        std::cerr << Color::color("[!] Invalid choice. Exiting...", Color::RED) << std::endl;
    }

    return 0;
}
