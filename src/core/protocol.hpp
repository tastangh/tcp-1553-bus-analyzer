#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <vector>

#pragma pack(push, 1)

// Our custom tcp payload structure (example, might need adjustments based on exact QEMU SIL Bridge specs)
struct Qemu1553Packet {
    uint8_t magic[2]; // 0xAA 0x55
    uint8_t mode;     // 0x01: RX, 0x02: TX
    uint8_t rt;
    uint8_t sa;
    uint8_t wc;
    uint16_t dataWords[32];
};

#pragma pack(pop)

struct MessageTransaction {
    uint64_t full_timetag = 0;
    uint32_t last_timetag_l_data = 0;
    uint32_t last_timetag_h_data = 0;
    uint16_t cmd1 = 0; char bus1 = 0; bool cmd1_valid = false;
    uint16_t cmd2 = 0; char bus2 = 0; bool cmd2_valid = false;
    uint16_t stat1 = 0; char stat1_bus = 0; bool stat1_valid = false;
    uint16_t stat2 = 0; char stat2_bus = 0; bool stat2_valid = false;
    std::vector<uint16_t> data_words;
    uint32_t error_word = 0; bool error_valid = false;

    void clear() {
        full_timetag = 0; last_timetag_l_data = 0; last_timetag_h_data = 0;
        cmd1 = 0; bus1 = 0; cmd1_valid = false;
        cmd2 = 0; bus2 = 0; cmd2_valid = false;
        stat1 = 0; stat1_bus = 0; stat1_valid = false;
        stat2 = 0; stat2_bus = 0; stat2_valid = false;
        data_words.clear();
        error_word = 0; error_valid = false;
    }
    
    bool isEmpty() const {
        return !cmd1_valid && !error_valid && full_timetag == 0 && data_words.empty();
    }
};

#endif // PROTOCOL_HPP
