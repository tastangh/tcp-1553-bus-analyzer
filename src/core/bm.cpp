#include "bm.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <set>

BM& BM::getInstance() {
    static BM instance;
    return instance;
}

BM::BM() : m_monitoringActive(false), m_shutdownRequested(false), m_dataLoggingEnabled(false),
           m_filterEnabled(false), m_filterBus(0), m_filterRt(-1), m_filterSa(-1), m_filterMc(-1) {
}

BM::~BM() {
    stop();
}

void BM::setUpdateMessagesCallback(UpdateMessagesCallback cb) {
    m_guiUpdateMessagesCb = cb;
}

void BM::setUpdateTreeItemCallback(UpdateTreeItemCallback cb) {
    m_guiUpdateTreeItemCb = cb;
}

bool BM::start(const std::string& host, uint16_t port, const std::string& remoteHost, uint16_t remotePort) {
    if (m_monitoringActive) return true;
    
    m_tcpProxy.terminate();
    
    // If the monitor thread exited on an error previously, it needs to be joined
    // before we can safely re-assign returning m_monitorThread
    if (m_monitorThread.joinable()) {
        m_monitorThread.join();
    }
    m_protocolParser.reset();
    {
        std::lock_guard<std::mutex> lock(m_guiBufferMutex);
        m_pendingMessages.clear();
        m_pendingActiveKeys.clear();
        m_lastGuiUpdate = std::chrono::steady_clock::now();
    }

    // Default to 'lo' if it looks like loopback, else use as interface name
    std::string interface = (host == "127.0.0.1" || host == "localhost") ? "lo" : host;
    
    Logger::info("Starting Passive Sniffer on " + interface + " port " + std::to_string(port));
    if (!m_tcpProxy.startSniffing(interface, port)) {
        return false;
    }

    m_currentPort = port;
    m_monitoringActive = true;
    m_shutdownRequested = false;
    m_monitorThread = std::thread(&BM::monitorThreadFunc, this);
    return true;
}

void BM::stop() {
    if (!m_monitoringActive) return;
    m_shutdownRequested = true;
    m_tcpProxy.terminate(); 
    if (m_monitorThread.joinable()) {
        m_monitorThread.join();
    }
    m_monitoringActive = false;
    m_shutdownRequested = false;
}

bool BM::isMonitoring() const {
    return m_monitoringActive;
}

void BM::enableFilter(bool enable) {
    m_filterEnabled = enable;
}

bool BM::isFilterEnabled() const {
    return m_filterEnabled;
}

void BM::setFilterCriteria(char bus, int rt, int sa, int mc) {
    std::lock_guard<std::mutex> lock(m_filterMutex);
    m_filterBus = bus;
    m_filterRt = rt;
    m_filterSa = sa;
    m_filterMc = mc;
}

void BM::enableDataLogging(bool enable) {
    m_dataLoggingEnabled = enable;
}

void BM::monitorThreadFunc() {
    unsigned char buffer[4096];
    while (!m_shutdownRequested) {
        int bytesRead = m_tcpProxy.receiveData(buffer, sizeof(buffer));
        if (bytesRead > 0) {
            processAndRelayData(buffer, bytesRead);
        }
        // Yield to other threads but don't sleep too long to avoid dropping packets
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    m_shutdownRequested = false; 
    m_monitoringActive = false;
    Logger::info("Monitor thread exiting.");
}

void BM::processAndRelayData(const unsigned char* buffer, uint32_t bytesRead) {
    m_protocolParser.parseStream(buffer, bytesRead, [&](const MessageTransaction& trans) {
        // Filter logic
        if (m_filterEnabled) {
            int rt = (trans.cmd1 >> 11) & 0x1F;
            int sa = (trans.cmd1 >> 5) & 0x1F;
            if (m_filterRt != -1 && rt != (int)m_filterRt) return;
            if (m_filterSa != -1 && sa != (int)m_filterSa) return;
        }

        std::string formatted;
        formatAndRelayTransaction(trans, formatted);
        
        std::lock_guard<std::mutex> lock(m_guiBufferMutex);
        m_pendingMessages += formatted;

        if (trans.cmd1_valid) {
            int rt = (trans.cmd1 >> 11) & 0x1F;
            int sa = (trans.cmd1 >> 5) & 0x1F;
            m_pendingActiveKeys.insert({trans.bus1, rt, sa});
        }
    });

    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastGuiUpdate).count() > 50) {
        std::string toSend;
        std::set<RtSaKey> keysToSend;
        {
            std::lock_guard<std::mutex> lock(m_guiBufferMutex);
            toSend = std::move(m_pendingMessages);
            keysToSend = std::move(m_pendingActiveKeys);
            m_pendingMessages.clear();
            m_pendingActiveKeys.clear();
        }

        if (m_guiUpdateMessagesCb && !toSend.empty()) {
            m_guiUpdateMessagesCb(toSend);
        }

        if (m_guiUpdateTreeItemCb && !keysToSend.empty()) {
            for (const auto& key : keysToSend) {
                m_guiUpdateTreeItemCb(key.bus, key.rt, key.sa, true);
            }
        }
        m_lastGuiUpdate = now;
    }
}

void BM::formatAndRelayTransaction(const MessageTransaction& trans, std::string& outString) {
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S") << "." 
       << std::setfill('0') << std::setw(3) 
       << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000
       << " | ";

    if (trans.cmd1_valid) {
        ss << "Bus " << trans.bus1 << " | CMD: " << std::hex << std::setw(4) << std::setfill('0') << trans.cmd1 << " | ";
        int rt = (trans.cmd1 >> 11) & 0x1F;
        int tr = (trans.cmd1 >> 10) & 0x01;
        int sa = (trans.cmd1 >> 5) & 0x1F;
        int wc = trans.cmd1 & 0x1F;
        ss << "RT: " << std::dec << rt << " " << (tr ? "TX" : "RX") << " SA: " << sa << " WC: " << (wc == 0 ? 32 : wc);
    }

    if (!trans.data_words.empty()) {
        ss << " | DATA: ";
        for (auto dw : trans.data_words) {
            ss << std::hex << std::setw(4) << std::setfill('0') << dw << " ";
        }
    }

    ss << "\n";
    outString = ss.str();
}
