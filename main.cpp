#include <iostream>
#include <fstream>
#include <string>
#include <filesystem> 
#include "color.hpp"
#include "server.hpp"
#include "client.hpp"

bool readFile(const std::string& filePath, std::vector<uint8_t>& fileData, std::string& fileName) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << Color::color("[-] Failed to open file: ", Color::RED) << filePath << std::endl;
        return false;
    }

    fileName = std::filesystem::path(filePath).filename().string();

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    fileData.resize(size);
    if (!file.read(reinterpret_cast<char*>(fileData.data()), size)) {
        std::cerr << Color::color("[-] Failed to read file data.", Color::RED) << std::endl;
        return false;
    }

    std::cout << Color::color("[+] File read successfully: ", Color::GREEN) << fileName << ", Size: " << size << " bytes" << std::endl;
    return true;
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << Color::color("[!] Incorrect number of arguments. Usage: [executable] [host] [port]", Color::RED) << std::endl;
        return 1;
    }

    std::string host = argv[1];
    int port = std::stoi(argv[2]);
    std::cout << Color::color("[i] Node started at ", Color::YELLOW) << host << ":" << port << std::endl;

    int mode;
    while (true) {
        std::cout << Color::color("[?] Please choose the operating mode", Color::CYAN) << std::endl;
        std::cout << Color::color("[?] 1. Sender", Color::CYAN) << std::endl;
        std::cout << Color::color("[?] 2. Receiver", Color::CYAN) << std::endl;
        std::cout << Color::color("[?] Input: ", Color::CYAN);
        std::cin >> mode;
        if (mode == 1 || mode == 2) break;
        std::cerr << Color::color("[!] Invalid choice. Try again.", Color::RED) << std::endl;
    }

    if (mode == 1) {
        Server server(host, port);
        std::cout << Color::color("[+] Node is now a sender", Color::GREEN) << std::endl;

        int sendMode;
        while (true) {
            std::cout << Color::color("[?] Please choose the sending mode", Color::CYAN) << std::endl;
            std::cout << Color::color("[?] 1. Text Input", Color::CYAN) << std::endl;
            std::cout << Color::color("[?] 2. File Input", Color::CYAN) << std::endl;
            std::cout << Color::color("[?] Input: ", Color::CYAN);
            std::cin >> sendMode;
            if (sendMode == 1 || sendMode == 2) break;
            std::cerr << Color::color("[!] Invalid choice. Try again.", Color::RED) << std::endl;
        }

        if (sendMode == 1) {
            std::cin.ignore();
            std::string userInput;
            std::cout << Color::color("[?] Please enter your input: ", Color::CYAN);
            std::getline(std::cin, userInput);
            server.setData(userInput);
        } else if (sendMode == 2) {
            std::cin.ignore();
            std::string filePath;
            std::cout << Color::color("[?] Please enter the file path: ", Color::CYAN);
            std::getline(std::cin, filePath);

            std::vector<uint8_t> fileData;
            std::string fileName;
            if (!readFile(filePath, fileData, fileName)) return 1;

            server.setData(fileData);
            server.setSendingFile(true);
            server.setFileName(fileName); 
        }

        server.run();
    } else if (mode == 2) {
        Client client(host, port);
        client.setDestination();
        client.run();
        std::cout << Color::color("[+] Node is now a receiver", Color::GREEN) << std::endl;
    }

    return 0;
}
