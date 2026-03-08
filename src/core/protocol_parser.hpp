#ifndef PROTOCOL_PARSER_HPP
#define PROTOCOL_PARSER_HPP

#include "protocol.hpp"
#include <vector>
#include <functional>

class ProtocolParser {
public:
    ProtocolParser();
    ~ProtocolParser();

    // Accumulates raw byte stream and parses valid messages.
    // Calls the provided callback for each decoded transaction.
    void parseStream(const unsigned char* buffer, int length, 
                     std::function<void(const struct MessageTransaction&)> onMessageDecoded);

    void reset();

private:
    std::vector<unsigned char> m_accumulator;
};

#endif // PROTOCOL_PARSER_HPP
