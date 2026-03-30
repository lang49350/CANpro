/*
 * UCAN-FD batch buffer implementation.
 */

#include "batch_buffer.h"

namespace {
static const uint8_t DLC_LEN_TABLE[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64
};
}

BatchBuffer::BatchBuffer(uint8_t busId)
    : m_busId(busId)
    , m_currentCount(0)
    , m_baseTimestamp(0)
    , m_flags(0)
    , m_batchStartTime(0)
    , m_totalFrames(0)
    , m_totalBatches(0)
    , m_maxFrames(BATCH_MAX_FRAMES)
    , m_timeoutMs(BATCH_TIMEOUT_MS)
    , m_currentByteSize(UCAN_HEADER_SIZE + UCAN_BATCH_HEADER_SIZE)
{
    memset(m_frames, 0, sizeof(m_frames));
}

void BatchBuffer::init() {
    reset();
}

void BatchBuffer::reset() {
    m_currentCount = 0;
    m_baseTimestamp = 0;
    m_flags = 0;
    m_batchStartTime = 0;
    m_currentByteSize = UCAN_HEADER_SIZE + UCAN_BATCH_HEADER_SIZE;
    memset(m_frames, 0, sizeof(m_frames));
}

uint8_t BatchBuffer::dlcToBytes(uint8_t dlc) {
    if (dlc >= 16) return 64;
    return DLC_LEN_TABLE[dlc];
}

uint8_t BatchBuffer::bytesToDLC(uint8_t bytes) {
    if (bytes <= 8) return bytes;
    if (bytes <= 12) return 9;
    if (bytes <= 16) return 10;
    if (bytes <= 20) return 11;
    if (bytes <= 24) return 12;
    if (bytes <= 32) return 13;
    if (bytes <= 48) return 14;
    return 15;
}

bool BatchBuffer::addFrame(uint32_t canId, uint8_t len, const uint8_t* data, uint64_t timestamp, uint8_t flags) {
    if (m_currentCount >= m_maxFrames || m_currentCount >= BATCH_MAX_FRAMES) {
        return false;
    }
    if (m_currentCount == 0) {
        m_baseTimestamp = timestamp;
        m_batchStartTime = timestamp;
    }

    const uint64_t offset = timestamp - m_baseTimestamp;
    if (offset > BATCH_MAX_TIME_OFFSET) {
        return false;
    }

    const uint8_t dlc = bytesToDLC(len);
    const uint8_t dataLen = dlcToBytes(dlc);
    const uint16_t framePacketSize = 7 + dataLen;
    if (m_currentByteSize + framePacketSize > UCAN_PACKET_MAX_SIZE) {
        return false;
    }

    UCANFrameData* frame = &m_frames[m_currentCount];
    frame->idFlags = canId;
    if (flags & 0x02) frame->idFlags |= UCAN_ID_FD_BIT;
    if (flags & 0x04) frame->idFlags |= UCAN_ID_BRS_BIT;
    if (flags & 0x08) frame->idFlags |= UCAN_ID_ESI_BIT;
    frame->meta = static_cast<uint8_t>(((m_busId & 0x03) << 6) | (dlc & 0x0F));
    frame->dtUs = static_cast<uint16_t>(offset);
    memcpy(frame->data, data, dataLen);

    ++m_currentCount;
    ++m_totalFrames;
    m_flags |= flags;
    m_currentByteSize += framePacketSize;
    return true;
}

bool BatchBuffer::shouldSend(uint64_t currentTime) {
    if (m_currentCount == 0) return false;
    if (m_currentCount >= m_maxFrames) return true;
    if (currentTime < m_batchStartTime) return true;
    const uint64_t elapsed = currentTime - m_batchStartTime;
    return elapsed >= (static_cast<uint64_t>(m_timeoutMs) * 1000ULL);
}

uint16_t BatchBuffer::buildPacket(uint8_t* buffer, uint16_t maxSize) {
    if (m_currentCount == 0) return 0;
    if (maxSize < (UCAN_HEADER_SIZE + UCAN_BATCH_HEADER_SIZE)) return 0;

    const uint8_t savedCount = m_currentCount;
    const uint32_t savedBaseTs = static_cast<uint32_t>(m_baseTimestamp & 0xFFFFFFFFULL);
    const uint8_t savedFlags = m_flags;
    const uint16_t savedByteSize = m_currentByteSize;
    UCANFrameData savedFrames[BATCH_MAX_FRAMES];
    memcpy(savedFrames, m_frames, sizeof(UCANFrameData) * savedCount);
    reset();

    uint16_t pos = UCAN_HEADER_SIZE;
    const uint16_t recordsBytes = static_cast<uint16_t>(savedByteSize - UCAN_HEADER_SIZE - UCAN_BATCH_HEADER_SIZE);

    buffer[pos++] = static_cast<uint8_t>(savedFlags | UCAN_BATCH_FLAG_HAS_RECORD_BYTES);
    buffer[pos++] = savedCount;
    buffer[pos++] = static_cast<uint8_t>(savedBaseTs & 0xFF);
    buffer[pos++] = static_cast<uint8_t>((savedBaseTs >> 8) & 0xFF);
    buffer[pos++] = static_cast<uint8_t>((savedBaseTs >> 16) & 0xFF);
    buffer[pos++] = static_cast<uint8_t>((savedBaseTs >> 24) & 0xFF);
    buffer[pos++] = 0;
    buffer[pos++] = 0;
    buffer[pos++] = static_cast<uint8_t>(recordsBytes & 0xFF);
    buffer[pos++] = static_cast<uint8_t>((recordsBytes >> 8) & 0xFF);

    for (uint8_t i = 0; i < savedCount; ++i) {
        UCANFrameData* frame = &savedFrames[i];
        const uint8_t dlc = frame->meta & 0x0F;
        const uint8_t dataLen = dlcToBytes(dlc);
        if (pos + 7 + dataLen > maxSize) {
            break;
        }

        const uint32_t idf = frame->idFlags;
        buffer[pos++] = static_cast<uint8_t>(idf & 0xFF);
        buffer[pos++] = static_cast<uint8_t>((idf >> 8) & 0xFF);
        buffer[pos++] = static_cast<uint8_t>((idf >> 16) & 0xFF);
        buffer[pos++] = static_cast<uint8_t>((idf >> 24) & 0xFF);
        buffer[pos++] = frame->meta;
        buffer[pos++] = static_cast<uint8_t>(frame->dtUs & 0xFF);
        buffer[pos++] = static_cast<uint8_t>((frame->dtUs >> 8) & 0xFF);
        memcpy(&buffer[pos], frame->data, dataLen);
        pos += dataLen;
    }

    const uint16_t payloadLen = static_cast<uint16_t>(pos - UCAN_HEADER_SIZE);
    buffer[0] = UCAN_MAGIC;
    buffer[1] = UCAN_MSG_RX_BATCH;
    buffer[2] = 0;
    buffer[3] = 0;
    buffer[4] = static_cast<uint8_t>(payloadLen & 0xFF);
    buffer[5] = static_cast<uint8_t>((payloadLen >> 8) & 0xFF);

    ++m_totalBatches;
    return pos;
}

void BatchBuffer::updateStrategy(uint32_t frameRate) {
    if (!USE_DYNAMIC_BATCH) return;
    if (frameRate > BATCH_HIGH_SPEED_THRESHOLD) {
        m_maxFrames = BATCH_HIGH_SPEED_FRAMES;
        m_timeoutMs = BATCH_HIGH_SPEED_TIMEOUT;
    } else if (frameRate > BATCH_MID_SPEED_THRESHOLD) {
        m_maxFrames = BATCH_MID_SPEED_FRAMES;
        m_timeoutMs = BATCH_MID_SPEED_TIMEOUT;
    } else {
        m_maxFrames = BATCH_LOW_SPEED_FRAMES;
        m_timeoutMs = BATCH_LOW_SPEED_TIMEOUT;
    }
}

bool BatchBuffer::isTimeOffsetOverflow(uint64_t timestamp) {
    if (m_currentCount == 0) return false;
    return (timestamp - m_baseTimestamp) > BATCH_MAX_TIME_OFFSET;
}
