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
    m_host = host;
    m_port = port;
    m_isInitialized = true;
    std::cout << "[BC] TCP Suite initialized for " << host << ":" << port << std::endl;
    return 0;
}

void BusController::shutdown() {
    std::lock_guard<std::mutex> lock(m_apiMutex);
    m_isInitialized = false;
}

bool BusController::isInitialized() const {
    return m_isInitialized;
}

int BusController::defineFrameResources(FrameComponent* frame) {
    // In TCP version, we don't need to define hardware resources.
    return 0;
}

int BusController::sendAcyclicFrame(FrameComponent* frame, std::array<uint16_t, BC_MAX_DATA_WORDS>& receivedData) {
    std::lock_guard<std::mutex> lock(m_apiMutex);
    if (!m_isInitialized) return -1;
    if (!frame) return -1;

    const FrameConfig& config = frame->getFrameConfig();
    
    Qemu1553Packet packet;
    packet.magic[0] = 0xAA;
    packet.magic[1] = 0x55;
    packet.mode = (config.mode == BcMode::BC_TO_RT) ? 0x00 : 0x01;
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
    size_t totalSize = 6 + (wordsToSend * 2);

    // Use the shared proxy from BM (which is the server)
    if (BM::getInstance().getTcpProxy().getConnectedClientCount() > 0) {
        BM::getInstance().getTcpProxy().transmitInjection(reinterpret_cast<uint8_t*>(&packet), totalSize);
    } else {
        std::cout << "[BC] Error: Not connected to Bridge" << std::endl;
    }

    return 0;
}

const char* BusController::getTcpError(int ret) {
    return "TCP Error or Not Connected";
}
