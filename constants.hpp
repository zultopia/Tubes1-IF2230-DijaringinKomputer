#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_RETRIES = 5;
constexpr int RETRANSMIT_DELAY_MS = 500; // in milliseconds

// TCP states
enum TcpState
{
    LISTEN,
    SYN_SENT,
    SYN_RECEIVED,
    ESTABLISHED,
    FIN_WAIT_1,
    FIN_WAIT_2,
    CLOSE_WAIT,
    CLOSING,
    LAST_ACK
};

#endif // CONSTANTS_HPP