/*
 * UCAN-FD zero-copy batch buffer.
 */

#include "zero_copy_buffer.h"

namespace {
static const uint8_t DLC_LEN_TABLE[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64
};

static uint8_t dlcToBytes(uint8_t dlc) {
    if (dlc >= 16) return 64;
    return DLC_LEN_TABLE[dlc];
}

static uint8_t bytesToDLC(uint8_t bytes) {
    if (bytes <= 8) return bytes;
    if (bytes <= 12) return 9;
    if (bytes <= 16) return 10;
    if (bytes <= 20) return 11;
    if (bytes <= 24) return 12;
    if (bytes <= 32) return 13;
    if (bytes <= 48) return 14;
    return 15;
}
}

ZeroCopyBatchBuffer::ZeroCopyBatchBuffer(uint8_t busId, uint8_t* usbBuffer, uint16_t bufferSize)
    : m_busId(busId)
    , m_usbBuffer(usbBuffer)
    , m_bufferSize(bufferSize)
    , m_currentOffset(0)
    , m_frameCount(0)
    , m_baseTimestamp(0)
    , m_batchStartTime(0)
    , m_totalFrames(0)
    , m_totalBatches(0)
    , m_maxFrames(BATCH_MAX_FRAMES)
    , m_timeoutMs(BATCH_TIMEOUT_MS)
{
}

void ZeroCopyBatchBuffer::init() { reset(); }

void ZeroCopyBatchBuffer::reset() {
    m_currentOffset = UCAN_HEADER_SIZE + UCAN_BATCH_HEADER_SIZE;
    m_frameCount = 0;
    m_baseTimestamp = 0;
    m_batchStartTime = 0;
}

void ZeroCopyBatchBuffer::writeHeader() {
    const uint32_t ts = static_cast<uint32_t>(m_baseTimestamp & 0xFFFFFFFFULL);
    m_usbBuffer[UCAN_HEADER_SIZE + 0] = UCAN_BATCH_FLAG_HAS_RECORD_BYTES;
    m_usbBuffer[UCAN_HEADER_SIZE + 1] = 0;
    m_usbBuffer[UCAN_HEADER_SIZE + 2] = static_cast<uint8_t>(ts & 0xFF);
    m_usbBuffer[UCAN_HEADER_SIZE + 3] = static_cast<uint8_t>((ts >> 8) & 0xFF);
    m_usbBuffer[UCAN_HEADER_SIZE + 4] = static_cast<uint8_t>((ts >> 16) & 0xFF);
    m_usbBuffer[UCAN_HEADER_SIZE + 5] = static_cast<uint8_t>((ts >> 24) & 0xFF);
    m_usbBuffer[UCAN_HEADER_SIZE + 6] = 0;
    m_usbBuffer[UCAN_HEADER_SIZE + 7] = 0;
    m_usbBuffer[UCAN_HEADER_SIZE + 8] = 0;
    m_usbBuffer[UCAN_HEADER_SIZE + 9] = 0;
}

void ZeroCopyBatchBuffer::updateFrameCount() {
    m_usbBuffer[UCAN_HEADER_SIZE + 1] = m_frameCount;
}

bool ZeroCopyBatchBuffer::addFrameDirect(uint32_t canId, uint8_t len, const uint8_t* data, uint64_t timestamp) {
    if (m_frameCount >= m_maxFrames || m_frameCount >= BATCH_MAX_FRAMES) return false;
    if (m_frameCount == 0) {
        m_baseTimestamp = timestamp;
        m_batchStartTime = timestamp;
        reset();
        writeHeader();
    }

    const uint64_t dt = timestamp - m_baseTimestamp;
    if (dt > BATCH_MAX_TIME_OFFSET) return false;

    const uint8_t dlc = bytesToDLC(len);
    const uint8_t dataLen = dlcToBytes(dlc);
    if (m_currentOffset + 7 + dataLen > m_bufferSize) return false;

    uint16_t p = m_currentOffset;
    m_usbBuffer[p++] = static_cast<uint8_t>(canId & 0xFF);
    m_usbBuffer[p++] = static_cast<uint8_t>((canId >> 8) & 0xFF);
    m_usbBuffer[p++] = static_cast<uint8_t>((canId >> 16) & 0xFF);
    m_usbBuffer[p++] = static_cast<uint8_t>((canId >> 24) & 0xFF);
    m_usbBuffer[p++] = static_cast<uint8_t>(((m_busId & 0x03) << 6) | (dlc & 0x0F));
    m_usbBuffer[p++] = static_cast<uint8_t>(dt & 0xFF);
    m_usbBuffer[p++] = static_cast<uint8_t>((dt >> 8) & 0xFF);
    memcpy(&m_usbBuffer[p], data, dataLen);
    m_currentOffset = static_cast<uint16_t>(p + dataLen);
    ++m_frameCount;
    ++m_totalFrames;
    return true;
}

bool ZeroCopyBatchBuffer::shouldSend(uint64_t currentTime) {
    if (m_frameCount == 0) return false;
    if (m_frameCount >= m_maxFrames) return true;
    if (currentTime < m_batchStartTime) return true;
    return (currentTime - m_batchStartTime) >= (static_cast<uint64_t>(m_timeoutMs) * 1000ULL);
}

uint16_t ZeroCopyBatchBuffer::finalizeBatch() {
    if (m_frameCount == 0) return 0;

    updateFrameCount();
    const uint16_t recordsBytes = static_cast<uint16_t>(m_currentOffset - UCAN_HEADER_SIZE - UCAN_BATCH_HEADER_SIZE);
    m_usbBuffer[UCAN_HEADER_SIZE + 8] = static_cast<uint8_t>(recordsBytes & 0xFF);
    m_usbBuffer[UCAN_HEADER_SIZE + 9] = static_cast<uint8_t>((recordsBytes >> 8) & 0xFF);
    const uint16_t payloadLen = static_cast<uint16_t>(m_currentOffset - UCAN_HEADER_SIZE);
    m_usbBuffer[0] = UCAN_MAGIC;
    m_usbBuffer[1] = UCAN_MSG_RX_BATCH;
    m_usbBuffer[2] = 0;
    m_usbBuffer[3] = 0;
    m_usbBuffer[4] = static_cast<uint8_t>(payloadLen & 0xFF);
    m_usbBuffer[5] = static_cast<uint8_t>((payloadLen >> 8) & 0xFF);

    const uint16_t size = m_currentOffset;
    ++m_totalBatches;
    reset();
    return size;
}

void ZeroCopyBatchBuffer::updateStrategy(uint32_t frameRate) {
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
