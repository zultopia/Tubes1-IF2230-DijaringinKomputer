# 🎌 IF2230 Project - TCP Over UDP 🎌  

👾 **Welcome to Networking Adventure!** 👾  
Greetings, brave coder-sensei! 🌸 This program is designed to facilitate file transfer over TCP sockets. It allows users to act as either a sender (server) or receiver (client), with additional support for sending files or text inputs.

---

## Team Members (DijaringinKomputer)
| NIM       | Name                             |
|-----------|----------------------------------|
| 13522070  | Marzuli Suhada M                 |
| 13522087  | Shulha                           |
| 13522108  | Muhammad Neo Cicero Koda         |
| 13522114  | Muhammad Dava Fathurrahman       |

---

## ⚔️ Key Features 

### **Modes of Operation**  
1. **Text Mode:** Send user-inputted text from sender to receiver.  
2. **File Mode:** Send raw binary data of any file type without corruption. 🎴  

### **Three-Way Handshake**  
Establish a secure connection using **SYN**, **SYN-ACK**, and **ACK messages**.  

### **Go-Back-N Mechanism**  
Ensure reliable delivery by resending lost or unacknowledged packets.  

### **Connection Termination**  
Terminate the session gracefully with a **FIN-ACK sequence**.  

### **Verbose Logs**  
Log key details like status, sequence numbers, acknowledgments, and error-handling messages for debugging.  

### **Bad Internet Simulation**  
Survive the trials of delay, packet loss, corruption, duplication, and reordering using network simulation commands.  

---

## Dependencies
Ensure you have the following installed:
- **C++ Compiler** (e.g., g++)
- Standard C++17 Library
- Networking capabilities on your machine

---

## File Structure

Tubes1-IF2230-DijaringinKomputer
├── .vscode
├── .gitignore
├── client.cpp
├── client.hpp
├── color.cpp
├── color.hpp
├── constants.hpp
├── main.cpp
├── Makefile
├── mc.cpp
├── ms.cpp
├── segment.cpp
├── segment.hpp
├── server.cpp
├── server.hpp
├── tcpsocket.cpp
└── tcpsocket.hpp

---

## 🌟 How to Play  

### **Setup**  
Clone this repository into your Linux environment:  
```bash
git clone https://github.com/zultopia/Tubes1-IF2230-DijaringinKomputer.git
```

---

## ⚔️ Compilation Instructions

### For Server
```bash
g++ ms.cpp server.cpp client.cpp tcpsocket.cpp segment.cpp color.cpp -o ms
```

### For Client
```bash
g++ mc.cpp server.cpp client.cpp tcpsocket.cpp segment.cpp color.cpp -o mc
```

### For Main Program
```bash
g++ main.cpp server.cpp client.cpp tcpsocket.cpp segment.cpp color.cpp -o main
```

## 🚀 Usage Instructions
1. Compile the project using the commands above.
2. Run the program using the following command (Replace <ip> and <port> with the desired host and port values):
```bash
./main <ip> <port>
```
3. Follow the on-screen instructions:
- Select operating mode (Sender or Receiver).
- For Sender: Choose to send text input or a file. Input the necessary data.
- For Receiver: Specify the destination and wait to receive data.

## 📝 Status Logs
Track the following during execution:
1. Current State: Handshake, Established, Closing, etc.
2. Sequence & Acknowledgment Numbers: Monitor segment flow.
3. Error Handling: Handle missing ACKs or timeouts.
4. Sliding Window Progress: Ensure smooth data flow.

## 🏷️ License
This project is licensed under the MIT License.

## 🌸 Let’s Make the Network Reliable! 🌸
Unleash your inner otaku, code like a pro, and bring honor to your clan by creating a reliable TCP over UDP protocol! 🚀