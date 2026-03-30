/*
 * ESP32-S3 ZCAN Gateway - CAN/CAN FD自动检测
 */

#ifndef AUTO_DETECT_H
#define AUTO_DETECT_H

#include <Arduino.h>
#include "../Config/config.h"
#include "../CANDriver/can_driver.h"

// 使用CANBusType命名空间中的BusType
using CANBusType::BusType;

// 自动检测器类
class AutoDetector {
public:
    AutoDetector();
    
    // 启动时检测
    BusType detectBusType(class CANDriver* driver, uint32_t timeoutMs = 10000);
    
    // 运行时监控
    void monitorBusType(class CANDriver* driver);
    
    // 获取检测结果
    BusType getBusType() const { return m_busType; }
    
    // 获取统计
    uint32_t getCAN20Count() const { return m_can20Count; }
    uint32_t getCANFDCount() const { return m_canfdCount; }
    
private:
    BusType m_busType;
    uint32_t m_can20Count;
    uint32_t m_canfdCount;
    
    // 判断是否为CAN FD帧
    bool isCANFDFrame(const CANFDMessage& msg);
};

#endif // AUTO_DETECT_H
