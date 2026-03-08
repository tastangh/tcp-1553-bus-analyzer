#ifndef BM_HPP
#define BM_HPP

#include "protocol.hpp"
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include "logger.hpp"
#include "tcp_proxy.hpp"
#include "protocol_parser.hpp"
#include <mutex>
#include <set>
#include <chrono>

class BM {
public:
    static BM& getInstance();

    BM(const BM&) = delete;
    BM& operator=(const BM&) = delete;

    using UpdateMessagesCallback = std::function<void(const std::string& formattedMessages)>;
    using UpdateTreeItemCallback = std::function<void(char bus, int rt, int sa, bool isActive)>;

    bool start(const std::string& host, uint16_t port, const std::string& remoteHost = "", uint16_t remotePort = 0);
    void stop();
    bool isMonitoring() const;

    void setUpdateMessagesCallback(UpdateMessagesCallback cb);
    void setUpdateTreeItemCallback(UpdateTreeItemCallback cb);

    void enableFilter(bool enable);
    bool isFilterEnabled() const;
    void setFilterCriteria(char bus, int rt, int sa, int mc = -1);
    void enableDataLogging(bool enable);

    TcpProxy& getTcpProxy() { return m_tcpProxy; }

private:
    BM();
    ~BM();

    void formatAndRelayTransaction(const MessageTransaction& trans, std::string& outString);

    void monitorThreadFunc();
    void processAndRelayData(const unsigned char* buffer, uint32_t bytesRead);
    bool initializeClient(const std::string& host, uint16_t port);

    TcpProxy m_tcpProxy;
    ProtocolParser m_protocolParser;
    uint16_t m_currentPort;

    std::thread m_monitorThread;
    std::atomic<bool> m_monitoringActive;
    std::atomic<bool> m_shutdownRequested;
    std::atomic<bool> m_dataLoggingEnabled; 

    UpdateMessagesCallback m_guiUpdateMessagesCb;
    UpdateTreeItemCallback m_guiUpdateTreeItemCb;

    std::atomic<bool> m_filterEnabled;
    std::atomic<char> m_filterBus;
    std::atomic<int>  m_filterRt;
    std::atomic<int>  m_filterSa;
    std::atomic<int>  m_filterMc;
    std::mutex        m_filterMutex;

    struct RtSaKey { 
        char bus; int rt; int sa;
        bool operator<(const RtSaKey& o) const {
            if (bus != o.bus) return bus < o.bus;
            if (rt != o.rt) return rt < o.rt;
            return sa < o.sa;
        }
    };

    std::chrono::steady_clock::time_point m_lastGuiUpdate;
    std::string m_pendingMessages;
    std::set<RtSaKey> m_pendingActiveKeys;
    std::mutex m_guiBufferMutex;
};

#endif // BM_HPP