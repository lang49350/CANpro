/*
 * ESP32-S3 ZCAN Gateway - CAN/CAN FD自动检测实现
 */

#include "auto_detect.h"

using CANBusType::BusType;
using CANBusType::BUS_UNKNOWN;
using CANBusType::BUS_CAN20_ONLY;
using CANBusType::BUS_CANFD_ONLY;
using CANBusType::BUS_MIXED;

AutoDetector::AutoDetector()
    : m_busType(BUS_UNKNOWN)
    , m_can20Count(0)
    , m_canfdCount(0)
{
}

BusType AutoDetector::detectBusType(CANDriver* driver, uint32_t timeoutMs) {
    if (!driver) {
        return BUS_UNKNOWN;
    }
    
    Serial.printf("[AutoDetect] 开始检测总线类型（超时: %lu ms）\n", timeoutMs);
    
    m_can20Count = 0;
    m_canfdCount = 0;
    
    uint64_t startTime = millis();
    CANFDMessage msgs[10];
    
    while (millis() - startTime < timeoutMs) {
        uint8_t count = driver->receiveBatch(msgs, 10);
        
        for (uint8_t i = 0; i < count; i++) {
            if (isCANFDFrame(msgs[i])) {
                m_canfdCount++;
            } else {
                m_can20Count++;
            }
        }
        
        // 如果已经检测到足够的帧，提前结束
        if (m_can20Count + m_canfdCount >= 100) {
            break;
        }
        
        delay(10);
    }
    
    // 判断总线类型
    if (m_canfdCount > 0 && m_can20Count > 0) {
        m_busType = BUS_MIXED;
        Serial.printf("[AutoDetect] ✅ 检测结果: 混合总线 (CAN2.0: %lu, CANFD: %lu)\n", 
                     m_can20Count, m_canfdCount);
    } else if (m_canfdCount > 0) {
        m_busType = BUS_CANFD_ONLY;
        Serial.printf("[AutoDetect] ✅ 检测结果: 纯CAN FD总线 (CANFD: %lu)\n", m_canfdCount);
    } else if (m_can20Count > 0) {
        m_busType = BUS_CAN20_ONLY;
        Serial.printf("[AutoDetect] ✅ 检测结果: 纯CAN 2.0总线 (CAN2.0: %lu)\n", m_can20Count);
    } else {
        m_busType = BUS_UNKNOWN;
        Serial.println("[AutoDetect] ⚠️  未检测到CAN帧");
    }
    
    return m_busType;
}

void AutoDetector::monitorBusType(CANDriver* driver) {
    // 运行时监控总线类型变化
    // 用于动态切换CAN/CAN FD模式
    
    if (!driver) {
        return;
    }
    
    static uint32_t lastCheckTime = 0;
    static uint32_t checkInterval = 5000;  // 每5秒检查一次
    
    uint32_t now = millis();
    if (now - lastCheckTime < checkInterval) {
        return;
    }
    lastCheckTime = now;
    
    // 重置计数器
    uint32_t oldCan20Count = m_can20Count;
    uint32_t oldCanfdCount = m_canfdCount;
    m_can20Count = 0;
    m_canfdCount = 0;
    
    // 采样100ms内的帧
    uint64_t sampleStart = millis();
    CANFDMessage msgs[10];
    
    while (millis() - sampleStart < 100) {
        uint8_t count = driver->receiveBatch(msgs, 10);
        
        for (uint8_t i = 0; i < count; i++) {
            if (isCANFDFrame(msgs[i])) {
                m_canfdCount++;
            } else {
                m_can20Count++;
            }
        }
        
        delay(5);
    }
    
    // 累加历史计数
    m_can20Count += oldCan20Count;
    m_canfdCount += oldCanfdCount;
    
    // 判断是否需要切换模式
    BusType newBusType = m_busType;
    
    if (m_canfdCount > 0 && m_can20Count > 0) {
        newBusType = BUS_MIXED;
    } else if (m_canfdCount > 0) {
        newBusType = BUS_CANFD_ONLY;
    } else if (m_can20Count > 0) {
        newBusType = BUS_CAN20_ONLY;
    }
    
    // 如果总线类型发生变化，打印日志
    if (newBusType != m_busType && newBusType != BUS_UNKNOWN) {
        Serial.printf("[AutoDetect] 总线类型变化: %d -> %d\n", m_busType, newBusType);
        m_busType = newBusType;
    }
}

bool AutoDetector::isCANFDFrame(const CANFDMessage& msg) {
    // 检查FDF位（CAN FD标志）
    // ACAN2517FD库中，len > 8表示CAN FD帧
    return msg.len > 8;
}
