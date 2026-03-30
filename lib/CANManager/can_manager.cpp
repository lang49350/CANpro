/*
 * ESP32-S3 ZCAN Gateway - CAN管理器实现
 */

#include "can_manager.h"
#include <SPI.h>
#include <ACAN2517FD.h>

// 外部SPI对象（在main.cpp中定义，改为指针）
extern SPIClass* can1SPI;
extern SPIClass* can2SPI;
extern ACAN2517FD* can1;
extern ACAN2517FD* can2;

CANManager::CANManager() {
    m_drivers[0] = nullptr;
    m_drivers[1] = nullptr;
    m_channelReady[0] = false;
    m_channelReady[1] = false;
}

CANManager::~CANManager() {
    if (m_drivers[0]) delete m_drivers[0];
    if (m_drivers[1]) delete m_drivers[1];
}

bool CANManager::init() {
    Serial0.println("\n========== CAN初始化开始 ==========");
    
    // 初始化CAN1
    m_channelReady[0] = initChannel(0);
    
    // 🔧 修复：在两个CAN初始化之间添加延迟，避免时序问题
    delay(100);  // 给CAN1足够的稳定时间
    
    // 初始化CAN2
    m_channelReady[1] = initChannel(1);
    
    Serial0.println("========== CAN初始化完成 ==========");
    
    if (m_channelReady[0] && m_channelReady[1]) {
        Serial0.println("✅ 双路CAN初始化成功");
        return true;
    } else if (m_channelReady[0]) {
        Serial0.println("⚠️  仅CAN1初始化成功");
        return true;
    } else if (m_channelReady[1]) {
        Serial0.println("⚠️  仅CAN2初始化成功");
        return true;
    } else {
        Serial0.println("❌ CAN初始化失败");
        return false;
    }
}

bool CANManager::initChannel(uint8_t channel) {
    if (channel > 1) {
        return false;
    }
    
    // 创建驱动对象（使用指针）
    if (channel == 0) {
        m_drivers[0] = new CANDriver(0, can1, can1SPI, CAN1_CS, CAN1_INT);
        return m_drivers[0]->init(CAN_AUTO_DETECT);
    } else {
        m_drivers[1] = new CANDriver(1, can2, can2SPI, CAN2_CS, CAN2_INT);
        return m_drivers[1]->init(CAN_AUTO_DETECT);
    }
}

CANDriver* CANManager::getDriver(uint8_t channel) {
    if (channel > 1) {
        return nullptr;
    }
    return m_drivers[channel];
}

bool CANManager::isChannelReady(uint8_t channel) const {
    if (channel > 1) {
        return false;
    }
    return m_channelReady[channel];
}

void CANManager::getStats(uint8_t channel, CANDriverStats& stats) {
    if (channel > 1 || !m_drivers[channel]) {
        memset(&stats, 0, sizeof(stats));
        return;
    }
    stats = m_drivers[channel]->getStats();
}
