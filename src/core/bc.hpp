#pragma once

#include "common.hpp"
#include <atomic>
#include <mutex>
#include <array>
#include <string>

class FrameComponent;

class BusController {
public:
    static BusController& getInstance();
    BusController(const BusController&) = delete;
    void operator=(const BusController&) = delete;

    int initialize(const std::string& host, uint16_t port);
    void shutdown();
    bool isInitialized() const;
    
    int defineFrameResources(FrameComponent* frame);
    int sendAcyclicFrame(FrameComponent* frame, std::array<uint16_t, BC_MAX_DATA_WORDS>& receivedData);
    
    static const char* getTcpError(int ret);

private:
    BusController() = default;
    ~BusController();

    std::mutex m_apiMutex;
    std::atomic<bool> m_isInitialized{false};
    std::string m_host;
    uint16_t m_port = 0;
};
