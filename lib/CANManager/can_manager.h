/*
 * ESP32-S3 ZCAN Gateway - CAN管理器
 * 统一管理双路CAN通道
 */

#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#include <Arduino.h>
#include "../Config/config.h"
#include "../CANDriver/can_driver.h"

// CAN管理器类
class CANManager {
public:
    CANManager();
    ~CANManager();
    
    // 初始化
    bool init();
    
    // 获取驱动
    CANDriver* getDriver(uint8_t channel);
    
    // 获取初始化状态
    bool isChannelReady(uint8_t channel) const;
    
    // 获取统计
    void getStats(uint8_t channel, CANDriverStats& stats);
    
private:
    CANDriver* m_drivers[2];
    bool m_channelReady[2];
    
    // 初始化单个通道
    bool initChannel(uint8_t channel);
};

#endif // CAN_MANAGER_H
