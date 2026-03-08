#include "tcp_proxy.hpp"
#include "logger.hpp"
#include <iostream>
#include <algorithm>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>

TcpProxy::TcpProxy() : m_pcapHandle(nullptr), m_running(false) {
}

TcpProxy::~TcpProxy() {
    terminate();
}

bool TcpProxy::startSniffing(const std::string& interface, uint16_t port) {
    char errbuf[PCAP_ERRBUF_SIZE];
    m_interface = interface;
    m_port = port;

    m_pcapHandle = pcap_open_live(interface.c_str(), 65535, 1, 100, errbuf);
    if (!m_pcapHandle) {
        Logger::error("Pcap open error: " + std::string(errbuf));
        return false;
    }

    struct bpf_program fp;
    std::string filter = "tcp port " + std::to_string(port);
    if (pcap_compile(m_pcapHandle, &fp, filter.c_str(), 0, PCAP_NETMASK_UNKNOWN) == -1) {
        Logger::error("Pcap filter compile error");
        pcap_close(m_pcapHandle);
        return false;
    }

    if (pcap_setfilter(m_pcapHandle, &fp) == -1) {
        Logger::error("Pcap filter set error");
        pcap_close(m_pcapHandle);
        return false;
    }

    m_running = true;
    m_captureThread = std::thread(&TcpProxy::captureLoop, this);
    Logger::info("Passive monitoring started on " + interface + " port " + std::to_string(port));
    return true;
}

void TcpProxy::terminate() {
    m_running = false;
    if (m_pcapHandle) {
        pcap_breakloop(m_pcapHandle);
    }
    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
    if (m_pcapHandle) {
        pcap_close(m_pcapHandle);
        m_pcapHandle = nullptr;
    }
}

int TcpProxy::getConnectedClientCount() const {
    // In passive mode, we don't track "clients", but we can return 1 if we are running
    return m_running ? 1 : 0;
}

int TcpProxy::receiveData(unsigned char* buffer, int maxBytes) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (m_payloadQueue.empty()) return 0;

    std::vector<unsigned char> payload = m_payloadQueue.front();
    m_payloadQueue.pop();

    int toCopy = std::min((int)payload.size(), maxBytes);
    memcpy(buffer, payload.data(), toCopy);
    return toCopy;
}

void TcpProxy::packetHandler(unsigned char* userData, const struct pcap_pkthdr* pkthdr, const unsigned char* packet) {
    TcpProxy* proxy = reinterpret_cast<TcpProxy*>(userData);
    
    int linkType = pcap_datalink(proxy->m_pcapHandle);
    int offset = 0;

    if (linkType == DLT_EN10MB) {
        offset = 14; // Ethernet
    } else if (linkType == DLT_NULL) {
        offset = 4; // BSD Loopback
    } else if (linkType == DLT_LINUX_SLL) {
        offset = 16; // Cooked
    } else {
        return; // Unknown link type
    }

    const struct ip* ipHeader = (const struct ip*)(packet + offset);
    int ipHeaderLen = ipHeader->ip_v == 4 ? ipHeader->ip_hl * 4 : 40; // Simplified IPv6 check

    const struct tcphdr* tcpHeader = (const struct tcphdr*)(packet + offset + ipHeaderLen);
    int tcpHeaderLen = tcpHeader->doff * 4;

    int payloadOffset = offset + ipHeaderLen + tcpHeaderLen;
    int payloadLen = pkthdr->caplen - payloadOffset;

    if (payloadLen > 0) {
        Logger::info("TcpProxy: Captured packet with TCP payload length: " + std::to_string(payloadLen));
        std::vector<unsigned char> payload(packet + payloadOffset, packet + payloadOffset + payloadLen);
        std::lock_guard<std::mutex> lock(proxy->m_queueMutex);
        proxy->m_payloadQueue.push(std::move(payload));
    }
}

void TcpProxy::captureLoop() {
    while (m_running) {
        pcap_dispatch(m_pcapHandle, -1, packetHandler, reinterpret_cast<unsigned char*>(this));
    }
}
