/*
 * ESP32-S3 ZCAN Gateway - 性能监控
 */

#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <Arduino.h>
#include "../Config/config.h"

// 性能监控器类
class PerformanceMonitor {
public:
    PerformanceMonitor();
    
    // 记录延迟
    void recordLatency(uint32_t latencyUs);
    
    // 获取平均延迟
    uint32_t getAvgLatency() const { return m_avgLatency; }
    
    // 获取最大延迟
    uint32_t getMaxLatency() const { return m_maxLatency; }
    
    // 重置统计
    void reset();
    
private:
    uint32_t m_avgLatency;
    uint32_t m_maxLatency;
    uint32_t m_totalLatency;
    uint32_t m_sampleCount;
};

#endif // PERFORMANCE_MONITOR_H
