/*
 * ESP32-S3 ZCAN Gateway - 任务管理器
 * 管理所有FreeRTOS任务的创建和调度
 */

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include "../Config/config.h"
#include "can_manager.h"

// 任务管理器类
class TaskManager {
public:
    TaskManager();
    
    // 初始化
    bool init(CANManager* canManager);
    
    // 创建所有任务
    bool createAllTasks();
    
    // 获取任务句柄
    TaskHandle_t getCAN1RxTaskHandle() const { return m_can1RxTaskHandle; }
    TaskHandle_t getCAN2RxTaskHandle() const { return m_can2RxTaskHandle; }
    
private:
    CANManager* m_canManager;
    
    // 任务句柄
    TaskHandle_t m_can1RxTaskHandle;
    TaskHandle_t m_can2RxTaskHandle;
    TaskHandle_t m_can1TxTaskHandle;
    TaskHandle_t m_can2TxTaskHandle;
    TaskHandle_t m_usbRxTaskHandle;
    TaskHandle_t m_usbTxTaskHandle;
    TaskHandle_t m_statsTaskHandle;
    
    // 创建各类任务
    bool createCANTasks();
    bool createUSBTasks();
    bool createStatsTasks();
};

#endif // TASK_MANAGER_H
