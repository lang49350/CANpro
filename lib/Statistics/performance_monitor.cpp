/*
 * ESP32-S3 ZCAN Gateway - 性能监控实现
 */

#include "performance_monitor.h"

PerformanceMonitor::PerformanceMonitor()
    : m_avgLatency(0)
    , m_maxLatency(0)
    , m_totalLatency(0)
    , m_sampleCount(0)
{
}

void PerformanceMonitor::recordLatency(uint32_t latencyUs) {
    m_totalLatency += latencyUs;
    m_sampleCount++;
    
    if (latencyUs > m_maxLatency) {
        m_maxLatency = latencyUs;
    }
    
    m_avgLatency = m_totalLatency / m_sampleCount;
}

void PerformanceMonitor::reset() {
    m_avgLatency = 0;
    m_maxLatency = 0;
    m_totalLatency = 0;
    m_sampleCount = 0;
}
