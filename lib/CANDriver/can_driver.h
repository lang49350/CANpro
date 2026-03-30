/*
 * ESP32-S3 ZCAN Gateway - CAN驱动
 * 基于ACAN2517FD库 + FIFO水位触发优化
 */

#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include <Arduino.h>
#include <SPI.h>
#include <ACAN2517FD.h>
#include "../Config/config.h"

// CAN总线类型（使用独立命名空间避免冲突）
namespace CANBusType {
    enum BusType {
        BUS_CAN20_ONLY,     // 纯CAN 2.0
        BUS_CANFD_ONLY,     // 纯CAN FD
        BUS_MIXED,          // 混合
        BUS_UNKNOWN         // 未知
    };
}
using CANBusType::BusType;

// 工作模式
enum WorkMode {
    WORK_MODE_NORMAL = 0,   // 收发并存 (TX=16, RX=16)
    WORK_MODE_RX_ONLY = 1,  // 纯接收 (TX=0, RX=31) - 优化接收，防止溢出
    WORK_MODE_TX_ONLY = 2   // 纯发送 (TX=31, RX=0) - 优化回放，最大化吞吐
};

// CAN驱动统计
struct CANDriverStats {
    uint32_t rxCount;           // 接收计数
    uint32_t txCount;           // 发送计数
    uint32_t errorCount;        // 错误计数
    uint32_t interruptCount;    // 中断计数
    uint32_t fifoOverflow;      // FIFO溢出
    BusType busType;            // 总线类型
};

class CANDriver {
public:
    CANDriver(uint8_t channelId, ACAN2517FD* can, SPIClass* spi,
              uint8_t csPin, uint8_t intPin);
    
    // 初始化
    bool init(bool autoDetect = true);
    
    // 启停控制
    bool start(uint32_t bitrate, uint32_t fdBitrate, bool isFD, WorkMode mode = WORK_MODE_NORMAL);
    void stop();

    // 中断处理
    // 设置接收回调（用于通知任务）
    void setNotificationCallback(void (*callback)());
    
    // 启用/禁用中断
    void enableInterrupt();
    void disableInterrupt();
    
    void handleInterrupt();
    
    // 批量接收（返回实际接收帧数）
    uint8_t receiveBatch(CANFDMessage* msgs, uint8_t maxCount);
    
    // 发送
    bool send(const CANFDMessage& msg);
    bool sendBatch(const CANFDMessage* msgs, uint8_t count);
    
    // 自动检测
    bool detectBusType();
    BusType getBusType() const { return m_stats.busType; }
    
    // 配置
    bool setBitrate(uint32_t bitrate);
    bool setFDBitrate(uint32_t dataBitrate);
    bool setMode(bool canFD);
    
    // 统计
    CANDriverStats getStats() const { return m_stats; }
    void resetStats();
    
    // 检查是否可用
    bool available() { return m_can->available(); }
    
    // 获取当前硬件TBC时间戳
    uint32_t currentTBC() { return m_can->currentTBC(); }
    
    // 调试：打印寄存器
    void printRegisters();
    
    // 强制轮询
    void poll();

    // 静态 ISR 包装器
    static void IRAM_ATTR can1ISR_Wrapper();
    static void IRAM_ATTR can2ISR_Wrapper();
    static CANDriver* instances[2];
    
private:
    uint8_t m_channelId;
    ACAN2517FD* m_can;
    SPIClass* m_spi;
    uint8_t m_csPin;
    uint8_t m_intPin;
    
    // 通知回调
    void (*m_notificationCallback)() = nullptr;
    
    CANDriverStats m_stats;

    // 保存的配置
    ACAN2517FDSettings::Oscillator m_oscillator;
    uint32_t m_spiFreq;
    
    // 尝试不同配置初始化
    bool tryInitialize();
    
    // 寄存器读写
    uint32_t readRegister32(uint16_t address);
    void writeRegister32(uint16_t address, uint32_t value);
};

#endif // CAN_DRIVER_H
