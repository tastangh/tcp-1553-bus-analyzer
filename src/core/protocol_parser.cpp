#include "protocol_parser.hpp"
#include "logger.hpp"
#include <cstring>
#include <arpa/inet.h>

ProtocolParser::ProtocolParser() {
}

ProtocolParser::~ProtocolParser() {
}

void ProtocolParser::parseStream(const unsigned char* buffer, int length, 
                                 std::function<void(const struct MessageTransaction&)> onMessageDecoded) 
{
    // Append new bytes
    m_accumulator.insert(m_accumulator.end(), buffer, buffer + length);

    // Header: AA 55 MODE RT SA WC (6 bytes)
    while (m_accumulator.size() >= 6) {
        // Look for magic sequence 0xAA 0x55
        if (m_accumulator[0] != 0xAA || m_accumulator[1] != 0x55) {
            Logger::warn("ProtocolParser: Skipping bad magic byte: 0x" + std::string(1, "0123456789ABCDEF"[m_accumulator[0] >> 4]) + std::string(1, "0123456789ABCDEF"[m_accumulator[0] & 0x0F]));
            m_accumulator.erase(m_accumulator.begin());
            continue;
        }

        uint8_t mode = m_accumulator[2];
        uint8_t rt   = m_accumulator[3];
        uint8_t sa   = m_accumulator[4];
        uint8_t wc   = m_accumulator[5];

        size_t words = (wc == 0) ? 32 : wc;
        size_t payloadSize = words * 2;
        size_t totalPacketSize = 6 + payloadSize;

        // Wait until we have the full packet
        if (m_accumulator.size() < totalPacketSize) {
            break; // Need more data
        }

        // We have a full packet. Decode it.
        MessageTransaction trans;
        trans.clear();
        
        // Mode 0: Receive (BC->RT), Mode 1: Transmit (RT->BC) in galacsim
        bool tr = (mode == 0x01); // 1 is Transmit (TX), 0 is Receive (RX)
        trans.cmd1 = (rt << 11) | (tr << 10) | (sa << 5) | (wc & 0x1F);
        trans.bus1 = 'A';
        trans.cmd1_valid = true;

        // Extract Data Words (Big-Endian from galacsim)
        const uint8_t* payloadPtr = m_accumulator.data() + 6;
        for (size_t i = 0; i < words; ++i) {
            uint16_t word = (payloadPtr[i*2] << 8) | payloadPtr[i*2 + 1];
            trans.data_words.push_back(word);
        }
        
        trans.stat1 = 0; // Simulated
        trans.stat1_valid = true;

        // Trigger callback
        if (onMessageDecoded) {
            onMessageDecoded(trans);
        }
        
        // Remove processed packet
        m_accumulator.erase(m_accumulator.begin(), m_accumulator.begin() + totalPacketSize);
    }
}

void ProtocolParser::reset() {
    m_accumulator.clear();
}
