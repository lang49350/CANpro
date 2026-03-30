/*
 * UCAN-FD command handler implementation.
 */

#include "ucan_handler.h"
#include "../CANManager/can_manager.h"

namespace {
static const uint8_t DLC_TO_BYTES_TABLE[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64
};

inline uint32_t readLe32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
        | (static_cast<uint32_t>(p[1]) << 8)
        | (static_cast<uint32_t>(p[2]) << 16)
        | (static_cast<uint32_t>(p[3]) << 24);
}

inline uint16_t readLe16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}
}

UCANHandler::UCANHandler()
    : m_sendQueue(nullptr)
    , m_freeQueue(nullptr)
    , m_usbToCanQueue1(nullptr)
    , m_usbToCanQueue2(nullptr)
    , m_highPriorityQueue(nullptr)
    , m_canManager(nullptr)
{
}

bool UCANHandler::init(QueueHandle_t sendQueue, QueueHandle_t freeQueue,
                        QueueHandle_t usbToCanQueue1, QueueHandle_t usbToCanQueue2) {
    m_sendQueue = sendQueue;
    m_freeQueue = freeQueue;
    m_usbToCanQueue1 = usbToCanQueue1;
    m_usbToCanQueue2 = usbToCanQueue2;

    if (!m_highPriorityQueue) {
        m_highPriorityQueue = xQueueCreate(64, sizeof(UCANPacket*));
    }
    return true;
}

void UCANHandler::setCANManager(CANManager* canManager) {
    m_canManager = canManager;
}

uint8_t UCANHandler::dlcToBytes(uint8_t dlc) {
    if (dlc >= 16) {
        return 64;
    }
    return DLC_TO_BYTES_TABLE[dlc];
}

bool UCANHandler::sendPacket(uint8_t type, uint8_t flags, uint8_t seq,
                              const uint8_t* payload, uint16_t payloadLen,
                              bool highPriority) {
    if (!m_freeQueue || !m_sendQueue || payloadLen > UCAN_MAX_PAYLOAD) {
        return false;
    }

    UCANPacket* packet = nullptr;
    if (xQueueReceive(m_freeQueue, &packet, pdMS_TO_TICKS(5)) != pdTRUE || !packet) {
        return false;
    }

    packet->data[0] = UCAN_MAGIC;
    packet->data[1] = type;
    packet->data[2] = flags;
    packet->data[3] = seq;
    packet->data[4] = static_cast<uint8_t>(payloadLen & 0xFF);
    packet->data[5] = static_cast<uint8_t>((payloadLen >> 8) & 0xFF);
    if (payload && payloadLen > 0) {
        memcpy(packet->data + UCAN_HEADER_SIZE, payload, payloadLen);
    }
    packet->length = static_cast<uint16_t>(UCAN_HEADER_SIZE + payloadLen);

    QueueHandle_t outQ = (highPriority && m_highPriorityQueue) ? m_highPriorityQueue : m_sendQueue;
    if (xQueueSend(outQ, &packet, pdMS_TO_TICKS(5)) != pdTRUE) {
        xQueueSend(m_freeQueue, &packet, 0);
        return false;
    }
    return true;
}

bool UCANHandler::sendAck(uint8_t reqType, uint8_t reqSeq, uint8_t status, uint8_t detail) {
    uint8_t p[4] = { reqType, reqSeq, status, detail };
    return sendPacket(UCAN_MSG_ACK, UCAN_FLAG_IS_RESP, reqSeq, p, sizeof(p), false);
}

void UCANHandler::handleHello(uint8_t flags, uint8_t seq) {
    uint8_t p[10];
    uint32_t caps = UCAN_CAP_CAN20 | UCAN_CAP_CANFD | UCAN_CAP_BATCH | UCAN_CAP_DUAL_CHANNEL;
    p[0] = UCAN_VERSION_MAJOR;
    p[1] = UCAN_VERSION_MINOR;
    p[2] = static_cast<uint8_t>(caps & 0xFF);
    p[3] = static_cast<uint8_t>((caps >> 8) & 0xFF);
    p[4] = static_cast<uint8_t>((caps >> 16) & 0xFF);
    p[5] = static_cast<uint8_t>((caps >> 24) & 0xFF);
    p[6] = 2;   // channels
    p[7] = 50;  // recommended max rx batch frames
    p[8] = 0;
    p[9] = 0;
    sendPacket(UCAN_MSG_HELLO_RSP, UCAN_FLAG_IS_RESP, seq, p, sizeof(p), true);
    if (flags & UCAN_FLAG_ACK_REQ) {
        sendAck(UCAN_MSG_HELLO_REQ, seq, UCAN_STATUS_OK, 0);
    }
}

void UCANHandler::handleChannelSet(uint8_t flags, uint8_t seq, const uint8_t* data, uint16_t len) {
    if (!data || len < 12) {
        sendAck(UCAN_MSG_CH_SET, seq, UCAN_STATUS_BAD_LEN, 0);
        return;
    }

    const uint8_t ch = data[0];
    if (ch >= 2 || !m_canManager) {
        sendAck(UCAN_MSG_CH_SET, seq, UCAN_STATUS_BAD_PARAM, 0);
        return;
    }

    const uint8_t modeBits = data[1];
    const uint8_t protoBits = data[2];
    const uint32_t nominal = readLe32(data + 4);
    const uint32_t dataBps = readLe32(data + 8);

    CANDriver* driver = m_canManager->getDriver(ch);
    if (!driver) {
        sendAck(UCAN_MSG_CH_SET, seq, UCAN_STATUS_INTERNAL_ERR, 0);
        return;
    }

    const bool enabled = (modeBits & UCAN_CH_MODE_EN) != 0;
    const bool rx = (modeBits & UCAN_CH_MODE_RX_EN) != 0;
    const bool tx = (modeBits & UCAN_CH_MODE_TX_EN) != 0;
    const bool isFD = (protoBits & UCAN_PROTO_CANFD_EN) != 0;
    const bool listenOnly = (modeBits & UCAN_CH_MODE_LISTEN_ONLY) != 0;

    bool ok = true;
    if (!enabled || (!rx && !tx)) {
        driver->stop();
    } else {
        WorkMode wm = WORK_MODE_NORMAL;
        if (listenOnly || (rx && !tx)) {
            wm = WORK_MODE_RX_ONLY;
        } else if (!rx && tx) {
            wm = WORK_MODE_TX_ONLY;
        }
        ok = driver->start(nominal, dataBps == 0 ? 2000000 : dataBps, isFD, wm);
    }

    if (ok) {
        m_channelCfg[ch].enabled = enabled;
        m_channelCfg[ch].modeBits = modeBits;
        m_channelCfg[ch].protoBits = protoBits;
        m_channelCfg[ch].nominalBps = nominal;
        m_channelCfg[ch].dataBps = dataBps == 0 ? 2000000 : dataBps;
    }

    if (flags & UCAN_FLAG_ACK_REQ) {
        sendAck(UCAN_MSG_CH_SET, seq, ok ? UCAN_STATUS_OK : UCAN_STATUS_INTERNAL_ERR, ch);
    }
}

void UCANHandler::handleChannelGet(uint8_t flags, uint8_t seq, const uint8_t* data, uint16_t len) {
    if (!data || len < 1 || data[0] >= 2) {
        sendAck(UCAN_MSG_CH_GET, seq, UCAN_STATUS_BAD_PARAM, 0);
        return;
    }

    const uint8_t ch = data[0];
    const ChannelConfig& cfg = m_channelCfg[ch];
    uint8_t p[14];
    p[0] = ch;
    p[1] = cfg.modeBits;
    p[2] = cfg.protoBits;
    p[3] = 0;
    p[4] = static_cast<uint8_t>(cfg.nominalBps & 0xFF);
    p[5] = static_cast<uint8_t>((cfg.nominalBps >> 8) & 0xFF);
    p[6] = static_cast<uint8_t>((cfg.nominalBps >> 16) & 0xFF);
    p[7] = static_cast<uint8_t>((cfg.nominalBps >> 24) & 0xFF);
    p[8] = static_cast<uint8_t>(cfg.dataBps & 0xFF);
    p[9] = static_cast<uint8_t>((cfg.dataBps >> 8) & 0xFF);
    p[10] = static_cast<uint8_t>((cfg.dataBps >> 16) & 0xFF);
    p[11] = static_cast<uint8_t>((cfg.dataBps >> 24) & 0xFF);
    p[12] = 0;
    p[13] = 0;
    sendPacket(UCAN_MSG_CH_STATE, UCAN_FLAG_IS_RESP, seq, p, sizeof(p), false);
    if (flags & UCAN_FLAG_ACK_REQ) {
        sendAck(UCAN_MSG_CH_GET, seq, UCAN_STATUS_OK, ch);
    }
}

void UCANHandler::handleStreamSet(uint8_t flags, uint8_t seq, const uint8_t* data, uint16_t len) {
    (void)data;
    if (len < 6) {
        sendAck(UCAN_MSG_STREAM_SET, seq, UCAN_STATUS_BAD_LEN, 0);
        return;
    }

    if (flags & UCAN_FLAG_ACK_REQ) {
        sendAck(UCAN_MSG_STREAM_SET, seq, UCAN_STATUS_OK, 0);
    }
}

void UCANHandler::handleTxBatch(uint8_t flags, uint8_t seq, const uint8_t* data, uint16_t len) {
    if (!data || len < UCAN_BATCH_HEADER_SIZE) {
        if (flags & UCAN_FLAG_ACK_REQ) {
            sendAck(UCAN_MSG_TX_BATCH, seq, UCAN_STATUS_BAD_LEN, 0);
        }
        return;
    }

    const uint8_t batchFlags = data[0];
    if ((batchFlags & UCAN_BATCH_FLAG_HAS_RECORD_BYTES) == 0) {
        if (flags & UCAN_FLAG_ACK_REQ) {
            sendAck(UCAN_MSG_TX_BATCH, seq, UCAN_STATUS_BAD_LEN, 0);
        }
        return;
    }
    const uint8_t frameCount = data[1];
    const uint16_t recordsBytes = readLe16(data + 8);
    if ((recordsBytes + UCAN_BATCH_HEADER_SIZE) != len) {
        if (flags & UCAN_FLAG_ACK_REQ) {
            sendAck(UCAN_MSG_TX_BATCH, seq, UCAN_STATUS_BAD_LEN, 0);
        }
        return;
    }

    const uint8_t* p = data + UCAN_BATCH_HEADER_SIZE;
    const uint8_t* recordsEnd = p + recordsBytes;
    uint16_t dropped = 0;

    for (uint8_t i = 0; i < frameCount; ++i) {
        if (recordsEnd - p < 7) {
            break;
        }
        const uint32_t idFlags = readLe32(p);
        const uint8_t meta = p[4];
        const uint8_t ch = (meta >> 6) & 0x03;
        const uint8_t dlc = meta & 0x0F;
        const uint8_t bytes = dlcToBytes(dlc);
        (void)readLe16(p + 5); // dt_us for scheduled mode (reserved for now)
        p += 7;
        if (recordsEnd - p < bytes) {
            break;
        }

        CANFDMessage msg;
        msg.id = idFlags & 0x1FFFFFFFUL;
        msg.ext = (idFlags & UCAN_ID_EXT_BIT) != 0;
        msg.len = bytes;
        memcpy(msg.data, p, bytes);

        const bool isFD = (idFlags & UCAN_ID_FD_BIT) != 0;
        const bool brs = (idFlags & UCAN_ID_BRS_BIT) != 0;
        const bool rtr = (idFlags & UCAN_ID_RTR_BIT) != 0;
        if (rtr) {
            msg.type = CANFDMessage::CAN_REMOTE;
        } else if (isFD) {
            msg.type = brs ? CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH
                           : CANFDMessage::CANFD_NO_BIT_RATE_SWITCH;
        } else {
            msg.type = CANFDMessage::CAN_DATA;
        }

        QueueHandle_t q = nullptr;
        if (ch == 0) q = m_usbToCanQueue1;
        if (ch == 1) q = m_usbToCanQueue2;
        if (!q || xQueueSend(q, &msg, 0) != pdTRUE) {
            ++dropped;
        }
        p += bytes;
    }

    if (flags & UCAN_FLAG_ACK_REQ) {
        sendAck(UCAN_MSG_TX_BATCH, seq, dropped == 0 ? UCAN_STATUS_OK : UCAN_STATUS_QUEUE_FULL,
                static_cast<uint8_t>(dropped > 255 ? 255 : dropped));
    }
}

void UCANHandler::handleStatsGet(uint8_t flags, uint8_t seq) {
    if (!m_canManager) {
        if (flags & UCAN_FLAG_ACK_REQ) {
            sendAck(UCAN_MSG_STATS_GET, seq, UCAN_STATUS_INTERNAL_ERR, 0);
        }
        return;
    }

    CANDriverStats s0{};
    CANDriverStats s1{};
    m_canManager->getStats(0, s0);
    m_canManager->getStats(1, s1);

    uint8_t p[24];
    auto wr32 = [&p](int o, uint32_t v) {
        p[o] = static_cast<uint8_t>(v & 0xFF);
        p[o + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
        p[o + 2] = static_cast<uint8_t>((v >> 16) & 0xFF);
        p[o + 3] = static_cast<uint8_t>((v >> 24) & 0xFF);
    };
    wr32(0, s0.rxCount); wr32(4, s0.txCount); wr32(8, s0.errorCount);
    wr32(12, s1.rxCount); wr32(16, s1.txCount); wr32(20, s1.errorCount);
    sendPacket(UCAN_MSG_STATS_RSP, UCAN_FLAG_IS_RESP, seq, p, sizeof(p), false);
    if (flags & UCAN_FLAG_ACK_REQ) {
        sendAck(UCAN_MSG_STATS_GET, seq, UCAN_STATUS_OK, 0);
    }
}

void UCANHandler::processCommand(uint8_t type, uint8_t flags, uint8_t seq,
                                  const uint8_t* data, uint16_t len) {
    switch (type) {
    case UCAN_MSG_HELLO_REQ:
        handleHello(flags, seq);
        break;
    case UCAN_MSG_CH_SET:
        handleChannelSet(flags, seq, data, len);
        break;
    case UCAN_MSG_CH_GET:
        handleChannelGet(flags, seq, data, len);
        break;
    case UCAN_MSG_STREAM_SET:
        handleStreamSet(flags, seq, data, len);
        break;
    case UCAN_MSG_TX_BATCH:
        handleTxBatch(flags, seq, data, len);
        break;
    case UCAN_MSG_STATS_GET:
        handleStatsGet(flags, seq);
        break;
    default:
        if (flags & UCAN_FLAG_ACK_REQ) {
            sendAck(type, seq, UCAN_STATUS_UNSUPPORTED, 0);
        }
        break;
    }
}
