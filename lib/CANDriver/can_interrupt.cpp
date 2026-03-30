/*
 * ESP32-S3 ZCAN Gateway - CAN中断处理实现
 */

#include "can_interrupt.h"

// 静态成员初始化
TaskHandle_t CANInterruptHandler::s_can1RxTaskHandle = NULL;
TaskHandle_t CANInterruptHandler::s_can2RxTaskHandle = NULL;

void CANInterruptHandler::init() {
    s_can1RxTaskHandle = NULL;
    s_can2RxTaskHandle = NULL;
}

void CANInterruptHandler::setCAN1TaskHandle(TaskHandle_t handle) {
    s_can1RxTaskHandle = handle;
}

void CANInterruptHandler::setCAN2TaskHandle(TaskHandle_t handle) {
    s_can2RxTaskHandle = handle;
}

void IRAM_ATTR CANInterruptHandler::can1ISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (s_can1RxTaskHandle != NULL) {
        vTaskNotifyGiveFromISR(s_can1RxTaskHandle, &xHigherPriorityTaskWoken);
    }
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void IRAM_ATTR CANInterruptHandler::can2ISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (s_can2RxTaskHandle != NULL) {
        vTaskNotifyGiveFromISR(s_can2RxTaskHandle, &xHigherPriorityTaskWoken);
    }
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
