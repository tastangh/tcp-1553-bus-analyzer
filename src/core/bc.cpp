#include "bc.hpp"
#include "FrameComponent.hpp"
#include "tcp_proxy.hpp"
#include "protocol.hpp"
#include "bm.hpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>

BusController& BusController::getInstance() {
    static BusController instance;
    return instance;
}

BusController::~BusController() {
    shutdown();
}

int BusController::initialize(const std::string& host, uint16_t port) {
    std::lock_guard<std::mutex> lock(m_apiMutex);
    
    if (m_isInitialized) return 0;

    m_host = host;
    m_port = port;

    m_serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverFd < 0) {
        std::cerr << "[BC] Error: Cannot create server socket." << std::endl;
        return -1;
    }

    int opt = 1;
    if (setsockopt(m_serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "[BC] Error: setsockopt failed." << std::endl;
        close(m_serverFd);
        m_serverFd = -1;
        return -1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    // We bind to INADDR_ANY to catch connections from loopback or externals
    serverAddr.sin_addr.s_addr = INADDR_ANY; 
    serverAddr.sin_port = htons(m_port);

    if (bind(m_serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        if (errno == EADDRINUSE) {
            m_lastError = "Cannot bind to port " + std::to_string(m_port) + ". QEMU is already running and using the bus.";
        } else {
            m_lastError = std::string("BC Server Bind Error: ") + strerror(errno);
        }
        std::cerr << "[BC] " << m_lastError << std::endl;
        close(m_serverFd);
        m_serverFd = -1;
        return -1;
    }

    if (listen(m_serverFd, 1) < 0) {
        m_lastError = std::string("BC Server Listen Error: ") + strerror(errno);
        std::cerr << "[BC] " << m_lastError << std::endl;
        close(m_serverFd);
        m_serverFd = -1;
        return -1;
    }

    m_lastError = "Ready";

    std::cout << "[BC] TCP Server listening on port " << m_port << ". Waiting for Simulator to connect..." << std::endl;
    m_isInitialized = true;

    // Start background thread to accept exactly one client at a time
    m_acceptThread = std::thread(&BusController::acceptLoop, this);
    
    return 0;
}

void BusController::acceptLoop() {
    while (m_isInitialized && m_serverFd >= 0) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        // Accept blocks until a connection comes in or the socket is closed (which interrupts it)
        int newSocket = accept(m_serverFd, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (!m_isInitialized) {
            if (newSocket >= 0) close(newSocket);
            break;
        }

        if (newSocket < 0) {
            // Accept failed, could be due to shutdown. Just loop again and check condition.
            continue;
        }

        // If we already have a client, we could close the old one or refuse new.
        // Let's replace the old client to allow simulator restarts.
        int oldClient = m_clientFd.exchange(newSocket);
        if (oldClient >= 0) {
            close(oldClient);
        }
        
        std::cout << "[BC] Simulator connected to BusController server!" << std::endl;
    }
}

void BusController::shutdown() {
    m_isInitialized = false;
    
    // Close sockets first to interrupt the accept thread
    if (m_serverFd >= 0) {
        // Use standard shutdown() API from <sys/socket.h> just for clarity 
        ::shutdown(m_serverFd, SHUT_RDWR);
        close(m_serverFd);
        m_serverFd = -1;
    }
    
    int cFd = m_clientFd.exchange(-1);
    if (cFd >= 0) {
        ::shutdown(cFd, SHUT_RDWR);
        close(cFd);
    }
    
    if (m_acceptThread.joinable()) {
        m_acceptThread.join();
    }
}

bool BusController::isInitialized() const {
    return m_isInitialized && (m_serverFd >= 0);
}

int BusController::defineFrameResources(FrameComponent* frame) {
    return 0;
}

int BusController::sendAcyclicFrame(FrameComponent* frame, std::array<uint16_t, BC_MAX_DATA_WORDS>& receivedData) {
    std::lock_guard<std::mutex> lock(m_apiMutex);
    
    int currentClientFd = m_clientFd.load();
    if (currentClientFd < 0) {
        std::cerr << "[BC] Error: Simulator is not connected yet. Waiting for connection." << std::endl;
        return -1;
    }
    if (!frame) return -1;

    const FrameConfig& config = frame->getFrameConfig();
    
    Qemu1553Packet packet;
    packet.magic[0] = 0xAA;
    packet.magic[1] = 0x55;
    
    // Simulator Protocol: 0x00 = Receive (BC->RT), 0x01 = Transmit (RT->BC)
    if (config.mode == BcMode::BC_TO_RT) {
        packet.mode = 0x00;
    } else if (config.mode == BcMode::RT_TO_BC) {
        packet.mode = 0x01;
    } else {
        packet.mode = 0x00; // Default
    }

    packet.rt = config.rt;
    packet.sa = config.sa;
    packet.wc = config.wc;

    for (int i = 0; i < 32; ++i) {
        try {
            if (i < (int)config.data.size()) {
                packet.dataWords[i] = htons((uint16_t)std::stoul(config.data[i], nullptr, 16));
            } else {
                packet.dataWords[i] = 0;
            }
        } catch (...) {
            packet.dataWords[i] = 0;
        }
    }

    size_t wordsToSend = (config.wc == 0) ? 32 : config.wc;
    // Simulator expects a full packet even for transmit commands to stay in sync.
    // We always send the header + 64 bytes of payload (zeroed out for RT_TO_BC)
    size_t totalSize = 6 + 64; 

    int bytesSent = write(currentClientFd, &packet, totalSize);
    if (bytesSent < 0) {
        std::cerr << "[BC] Error: Failed to send data to Simulator. Error: " << strerror(errno) << std::endl;
        int fdToClose = m_clientFd.exchange(-1);
        if (fdToClose == currentClientFd) close(currentClientFd);
        return -1;
    }

    // If it's a Transmit (RT->BC) command, wait for simulator to send data back
    if (config.mode == BcMode::RT_TO_BC) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms timeout
        setsockopt(currentClientFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        Qemu1553Packet response;
        int bytesReceived = read(currentClientFd, &response, sizeof(response));
        if (bytesReceived >= 6) {
            int receivedWords = (response.wc == 0) ? 32 : response.wc;
            for (int i = 0; i < receivedWords && i < 32; ++i) {
                receivedData[i] = ntohs(response.dataWords[i]);
            }
            return 0; // Success with data
        } else {
            // No response or error
            return -1;
        }
    }

    return 0;
}

const char* BusController::getTcpError(int ret) {
    return "TCP Error or Not Connected";
}
