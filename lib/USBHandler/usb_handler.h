/*
 * ESP32-S3 ZCAN Gateway - USB处理器
 * USB双缓冲（Ping-Pong Buffer）实现
 */

#ifndef USB_HANDLER_H
#define USB_HANDLER_H

#include <Arduino.h>
#include <USB.h>
#include <USBCDC.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../Config/config.h"

// USB双缓冲结构
struct USBDoubleBuffer {
    uint8_t bufferA[USB_BUFFER_SIZE];
    uint8_t bufferB[USB_BUFFER_SIZE];
    uint16_t sizeA;
    uint16_t sizeB;
    volatile bool usingA;
    SemaphoreHandle_t swapSemaphore;
    TaskHandle_t sendTaskHandle;
};

// USB处理器类
class USBHandler {
public:
    USBHandler(USBCDC* usbSerial);
    
    // 初始化
    bool init();
    
    // 双缓冲操作
    uint8_t* getFillBuffer();           // 获取填充缓冲区
    uint16_t getFillBufferSize();       // 获取填充缓冲区大小
    bool swapBuffers(uint16_t size);    // 交换缓冲区
    
    // 发送数据
    bool send(const uint8_t* data, uint16_t size);
    bool sendDirect();                  // 发送当前缓冲区（双缓冲模式）
    
    // 接收数据
    int available();
    uint8_t read();
    
    // 统计
    uint32_t getTotalBytesSent() const { return m_totalBytesSent; }
    uint32_t getTotalPacketsSent() const { return m_totalPacketsSent; }
    
    // 设置发送任务句柄
    void setSendTaskHandle(TaskHandle_t handle);
    
private:
    USBCDC* m_usbSerial;
    USBDoubleBuffer m_doubleBuffer;
    
    uint32_t m_totalBytesSent;
    uint32_t m_totalPacketsSent;
    
    // 实际发送函数
    bool sendBuffer(const uint8_t* buffer, uint16_t size);
};

#endif // USB_HANDLER_H
