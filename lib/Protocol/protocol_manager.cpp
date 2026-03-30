/*
 * UCAN-FD protocol manager.
 * Packet: magic(0xA6) + type + flags + seq + len16 + payload
 */

#include "protocol_manager.h"

ProtocolManager::ProtocolManager()
    : m_state(STATE_IDLE)
    , m_totalCommands(0)
    , m_currentType(0)
    , m_currentFlags(0)
    , m_currentSeq(0)
    , m_bufferIdx(0)
    , m_expectedLen(0)
{
}

bool ProtocolManager::init() {
    return true;
}

void ProtocolManager::dispatchPacket() {
    m_ucanHandler.processCommand(
        m_currentType,
        m_currentFlags,
        m_currentSeq,
        m_buffer,
        m_bufferIdx
    );
    ++m_totalCommands;
}

void ProtocolManager::processByte(uint8_t byte) {
    switch (m_state) {
        case STATE_IDLE:
            if (byte == UCAN_MAGIC) {
                m_state = STATE_TYPE;
            }
            break;

        case STATE_TYPE:
            m_currentType = byte;
            m_state = STATE_FLAGS;
            break;

        case STATE_FLAGS:
            m_currentFlags = byte;
            m_state = STATE_SEQ;
            break;

        case STATE_SEQ:
            m_currentSeq = byte;
            m_state = STATE_LEN_LO;
            break;

        case STATE_LEN_LO:
            m_expectedLen = byte;
            m_state = STATE_LEN_HI;
            break;

        case STATE_LEN_HI:
            m_expectedLen |= static_cast<uint16_t>(byte) << 8;
            m_bufferIdx = 0;
            if (m_expectedLen == 0) {
                dispatchPacket();
                m_state = STATE_IDLE;
            } else if (m_expectedLen > sizeof(m_buffer)) {
                m_state = STATE_IDLE;
            } else {
                m_state = STATE_DATA;
            }
            break;

        case STATE_DATA:
            if (m_bufferIdx < sizeof(m_buffer)) {
                m_buffer[m_bufferIdx++] = byte;
            } else {
                m_state = STATE_IDLE;
                break;
            }
            if (m_bufferIdx >= m_expectedLen) {
                dispatchPacket();
                m_state = STATE_IDLE;
            }
            break;

        default:
            m_state = STATE_IDLE;
            break;
    }
}
