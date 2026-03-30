/*
 * UCAN-FD TX/RX batch buffer.
 */

#ifndef BATCH_BUFFER_H
#define BATCH_BUFFER_H

#include <Arduino.h>
#include "../Config/config.h"

// 批量缓冲类
class BatchBuffer {
public:
    BatchBuffer(uint8_t busId);
    
    // 初始化
    void init();
    void reset();
    
    // 添加帧（零拷贝），支持flags
    bool addFrame(uint32_t canId, uint8_t len, const uint8_t* data, uint64_t timestamp, uint8_t flags = 0);
    
    // 检查是否需要发送
    bool shouldSend(uint64_t currentTime);
    
    // 构建批量帧数据包（零拷贝，直接写入目标缓冲区）
    uint16_t buildPacket(uint8_t* buffer, uint16_t maxSize);
    
    // 获取统计信息
    uint32_t getFrameCount() const { return m_currentCount; }
    uint32_t getTotalFrames() const { return m_totalFrames; }
    uint32_t getTotalBatches() const { return m_totalBatches; }
    
    // 动态批量策略
    void updateStrategy(uint32_t frameRate);
    
    // 配置参数
    void setMaxFrames(uint8_t maxFrames) { m_maxFrames = maxFrames; }
    void setTimeoutMs(uint16_t timeoutMs) { m_timeoutMs = timeoutMs; }
    uint8_t getMaxFrames() const { return m_maxFrames; }
    uint16_t getTimeoutMs() const { return m_timeoutMs; }
    
private:
    uint8_t m_busId;
    uint8_t m_currentCount;
    uint64_t m_baseTimestamp;  // 🔧 修复：改为64位，避免截断
    uint8_t m_flags;
    uint64_t m_batchStartTime;
    
    UCANFrameData m_frames[BATCH_MAX_FRAMES];
    
    // 统计
    uint32_t m_totalFrames;
    uint32_t m_totalBatches;
    
    // 动态批量策略参数
    uint8_t m_maxFrames;
    uint16_t m_timeoutMs;
    
    // 检查时间偏移是否溢出
    bool isTimeOffsetOverflow(uint64_t timestamp);

    // 辅助函数：DLC转换
    static uint8_t bytesToDLC(uint8_t bytes);
    static uint8_t dlcToBytes(uint8_t dlc);

    // 当前累计字节大小（用于防止包溢出）
    uint16_t m_currentByteSize;
};

#endif // BATCH_BUFFER_H
