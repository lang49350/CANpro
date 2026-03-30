/*
 * ESP32-S3 UCAN Gateway - 零拷贝缓冲区
 * 直接在USB发送缓冲区构建UCAN批量帧
 */

#ifndef ZERO_COPY_BUFFER_H
#define ZERO_COPY_BUFFER_H

#include <Arduino.h>
#include "../Config/config.h"
#include "batch_buffer.h"

// 零拷贝批量缓冲类
class ZeroCopyBatchBuffer {
public:
    ZeroCopyBatchBuffer(uint8_t busId, uint8_t* usbBuffer, uint16_t bufferSize);
    
    // 初始化
    void init();
    void reset();
    
    // 直接在USB缓冲区添加帧（零拷贝）
    bool addFrameDirect(uint32_t canId, uint8_t len, const uint8_t* data, uint64_t timestamp);
    
    // 检查是否需要发送
    bool shouldSend(uint64_t currentTime);
    
    // 获取当前数据包大小
    uint16_t getPacketSize() const { return m_currentOffset; }
    
    // 完成当前批次（返回数据包大小）
    uint16_t finalizeBatch();
    
    // 获取USB缓冲区指针
    uint8_t* getBuffer() const { return m_usbBuffer; }
    
    // 统计
    uint32_t getTotalFrames() const { return m_totalFrames; }
    uint32_t getTotalBatches() const { return m_totalBatches; }
    
    // 动态批量策略
    void updateStrategy(uint32_t frameRate);
    
private:
    uint8_t m_busId;
    uint8_t* m_usbBuffer;       // 直接指向USB发送缓冲区
    uint16_t m_bufferSize;
    uint16_t m_currentOffset;   // 当前写入位置
    
    uint8_t m_frameCount;       // 当前批次帧数
    uint32_t m_baseTimestamp;
    uint64_t m_batchStartTime;
    
    // 统计
    uint32_t m_totalFrames;
    uint32_t m_totalBatches;
    
    // 动态批量策略参数
    uint8_t m_maxFrames;
    uint16_t m_timeoutMs;
    
    // 写入批量帧头
    void writeHeader();
    
    // 更新帧数量字段
    void updateFrameCount();
};

#endif // ZERO_COPY_BUFFER_H
