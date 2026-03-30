/*
 * ESP32-S3 ZCAN Gateway - 性能统计
 */

#ifndef STATISTICS_H
#define STATISTICS_H

#include <Arduino.h>
#include "../Config/config.h"
#include "../CANManager/can_manager.h"

// 性能指标
struct PerformanceMetrics {
    // 吞吐量
    uint32_t can1FrameRate;      // 帧/秒
    uint32_t can2FrameRate;
    uint32_t totalFrameRate;
    
    // 延迟
    uint32_t avgLatency;         // 微秒
    uint32_t maxLatency;
    
    // 可靠性
    uint32_t droppedFrames;      // 丢帧数
    uint32_t errorCount;
    float reliability;           // 可靠性百分比
    
    // 资源占用
    float cpuUsage;              // CPU占用率
    uint32_t memoryUsage;        // 内存占用（字节）
    uint32_t queueDepth;         // 队列深度
    
    // 资源监控（新增）
    uint32_t poolUsage;          // 内存池使用率（%）
    uint32_t poolEmptyCount;     // 内存池耗尽次数
    uint32_t queueFullCount;     // 队列满次数
    uint32_t buildFailedCount;   // 构建失败次数
};

// 统计管理器类
class Statistics {
public:
    Statistics();
    
    // 初始化
    bool init(CANManager* canManager);
    
    // 更新统计
    void update();
    
    // 打印统计
    void print();
    
    // 获取指标
    const PerformanceMetrics& getMetrics() const { return m_metrics; }
    
    // 记录接收帧
    void recordRxFrame(uint8_t channel);
    
    // 记录发送帧
    void recordTxFrame(uint8_t channel);
    
    // 记录批次
    void recordBatch();
    
private:
    CANManager* m_canManager;
    PerformanceMetrics m_metrics;
    
    // 计数器
    uint32_t m_totalFramesReceived;
    uint32_t m_totalBatchesSent;
    uint32_t m_lastFrameCount;
    uint32_t m_lastCan1RxCount;
    uint32_t m_lastCan2RxCount;
    
    // 上次更新时间
    uint64_t m_lastUpdateTime;
};

#endif // STATISTICS_H
