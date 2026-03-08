#ifndef PCAP_SNIFFER_HPP
#define PCAP_SNIFFER_HPP

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <pcap.h>

class TcpProxy {
public:
    TcpProxy();
    ~TcpProxy();

    // interface: name of the dev (e.g. "lo"), port: tcp port to sniff (e.g. 5002)
    bool startSniffing(const std::string& interface, uint16_t port);
    void terminate();

    int getConnectedClientCount() const;
    
    // Reads captured payload data into buffer
    int receiveData(unsigned char* buffer, int maxBytes);
    
    // transmitInjection is disabled in passive mode (Read-only)
    bool transmitInjection(const unsigned char* buffer, int numBytes) { return false; }

private:
    static void packetHandler(unsigned char* userData, const struct pcap_pkthdr* pkthdr, const unsigned char* packet);
    void captureLoop();

    pcap_t* m_pcapHandle;
    std::string m_interface;
    uint16_t m_port;

    std::atomic<bool> m_running;
    std::thread m_captureThread;

    std::queue<std::vector<unsigned char>> m_payloadQueue;
    mutable std::mutex m_queueMutex;
};

#endif // PCAP_SNIFFER_HPP
