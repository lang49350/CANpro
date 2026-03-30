/*
 * ESP32-S3 ZCAN Gateway - CAN中断处理
 */

#ifndef CAN_INTERRUPT_H
#define CAN_INTERRUPT_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 中断处理器类
class CANInterruptHandler {
public:
    static void init();
    static void setCAN1TaskHandle(TaskHandle_t handle);
    static void setCAN2TaskHandle(TaskHandle_t handle);
    
    // ISR函数（必须是静态的）
    static void IRAM_ATTR can1ISR();
    static void IRAM_ATTR can2ISR();
    
private:
    static TaskHandle_t s_can1RxTaskHandle;
    static TaskHandle_t s_can2RxTaskHandle;
};

#endif // CAN_INTERRUPT_H
