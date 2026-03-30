/*
 * UCAN-FD protocol manager (single supported protocol).
 */

#ifndef PROTOCOL_MANAGER_H
#define PROTOCOL_MANAGER_H

#include <Arduino.h>
#include "../Config/config.h"
#include "ucan_handler.h"

class ProtocolManager {
public:
    ProtocolManager();

    bool init();

    void processByte(uint8_t byte);

    UCANHandler* getUCANHandler() { return &m_ucanHandler; }

    uint32_t getTotalCommands() const { return m_totalCommands; }

private:
    UCANHandler m_ucanHandler;

    enum State {
        STATE_IDLE,
        STATE_TYPE,
        STATE_FLAGS,
        STATE_SEQ,
        STATE_LEN_LO,
        STATE_LEN_HI,
        STATE_DATA
    };

    State m_state;
    uint32_t m_totalCommands;

    uint8_t m_currentType;
    uint8_t m_currentFlags;
    uint8_t m_currentSeq;
    uint8_t m_buffer[UCAN_PACKET_MAX_SIZE];
    uint16_t m_bufferIdx;
    uint16_t m_expectedLen;

    void dispatchPacket();
};

#endif // PROTOCOL_MANAGER_H
