/*
 * ESP32-S3 ZCAN Gateway - 任务管理器实现
 */

#include "task_manager.h"

// 任务函数声明（在main.cpp中实现）
extern void canReceiveTask(void *parameter);
extern void canTransmitTask(void *parameter);
extern void usbReceiveTask(void *parameter);
extern void usbSendTask(void *parameter);
extern void statisticsTask(void *parameter);

TaskManager::TaskManager()
    : m_canManager(nullptr)
    , m_can1RxTaskHandle(nullptr)
    , m_can2RxTaskHandle(nullptr)
    , m_can1TxTaskHandle(nullptr)
    , m_can2TxTaskHandle(nullptr)
    , m_usbRxTaskHandle(nullptr)
    , m_usbTxTaskHandle(nullptr)
    , m_statsTaskHandle(nullptr)
{
}

bool TaskManager::init(CANManager* canManager) {
    m_canManager = canManager;
    return true;
}

bool TaskManager::createAllTasks() {
    Serial0.println("\n========== 创建FreeRTOS任务 ==========");
    
    // 创建USB任务（Core 0 - 协议核心）
    if (!createUSBTasks()) {
        Serial0.println("❌ USB任务创建失败");
        return false;
    }
    
    // 创建统计任务（Core 0）
    if (!createStatsTasks()) {
        Serial0.println("❌ 统计任务创建失败");
        return false;
    }
    
    // 创建CAN任务（Core 1 - CAN核心）
    if (!createCANTasks()) {
        Serial0.println("❌ CAN任务创建失败");
        return false;
    }
    
    Serial0.println("========== 任务创建完成 ==========");
    Serial0.println("✅ 所有任务已创建");
    
    return true;
}

bool TaskManager::createUSBTasks() {
    // USB接收任务
    BaseType_t result = xTaskCreatePinnedToCore(
        usbReceiveTask,
        "USBRx",
        TASK_STACK_SIZE_USB_RX,
        NULL,
        TASK_PRIORITY_USB_RX,
        &m_usbRxTaskHandle,
        0  // Core 0
    );
    
    if (result != pdPASS) {
        return false;
    }
    
    // USB发送任务
    result = xTaskCreatePinnedToCore(
        usbSendTask,
        "USBTx",
        TASK_STACK_SIZE_USB_TX,
        NULL,
        TASK_PRIORITY_USB_TX,
        &m_usbTxTaskHandle,
        0  // Core 0
    );
    
    return (result == pdPASS);
}

bool TaskManager::createStatsTasks() {
    BaseType_t result = xTaskCreatePinnedToCore(
        statisticsTask,
        "Stats",
        TASK_STACK_SIZE_STATISTICS,
        NULL,
        TASK_PRIORITY_STATISTICS,
        &m_statsTaskHandle,
        0  // Core 0
    );
    
    return (result == pdPASS);
}

bool TaskManager::createCANTasks() {
    if (!m_canManager) {
        return false;
    }
    
    // CAN1接收任务
    if (m_canManager->isChannelReady(0)) {
        BaseType_t result = xTaskCreatePinnedToCore(
            canReceiveTask,
            "CAN1Rx",
            TASK_STACK_SIZE_CAN_RX,
            (void*)0,  // 通道0
            TASK_PRIORITY_CAN_BATCH,
            &m_can1RxTaskHandle,
            1  // Core 1
        );
        
        if (result != pdPASS) {
            return false;
        }
        
        // CAN1发送任务
        result = xTaskCreatePinnedToCore(
            canTransmitTask,
            "CAN1Tx",
            TASK_STACK_SIZE_CAN_TX,
            (void*)0,  // 通道0
            TASK_PRIORITY_CAN_TX,
            &m_can1TxTaskHandle,
            1  // Core 1
        );
        
        if (result != pdPASS) {
            return false;
        }
    }
    
    // CAN2接收任务
    if (m_canManager->isChannelReady(1)) {
        BaseType_t result = xTaskCreatePinnedToCore(
            canReceiveTask,
            "CAN2Rx",
            TASK_STACK_SIZE_CAN_RX,
            (void*)1,  // 通道1
            TASK_PRIORITY_CAN_BATCH,
            &m_can2RxTaskHandle,
            1  // Core 1
        );
        
        if (result != pdPASS) {
            return false;
        }
        
        // CAN2发送任务
        result = xTaskCreatePinnedToCore(
            canTransmitTask,
            "CAN2Tx",
            TASK_STACK_SIZE_CAN_TX,
            (void*)1,  // 通道1
            TASK_PRIORITY_CAN_TX,
            &m_can2TxTaskHandle,
            1  // Core 1
        );
        
        if (result != pdPASS) {
            return false;
        }
    }
    
    return true;
}
