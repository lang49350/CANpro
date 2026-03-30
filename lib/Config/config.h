/*
 * ESP32-S3 ZCAN Gateway - 全局配置
 * 版本: v4.2
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "hardware_config.h"
#include "performance_config.h"
#include "protocol_config.h"

// 版本信息
#define FIRMWARE_VERSION_MAJOR  5
#define FIRMWARE_VERSION_MINOR  8
#define FIRMWARE_VERSION_PATCH  0
#define FIRMWARE_BUILD_NUM      336

// 调试配置
#ifndef APP_DEBUG_MODE
#define APP_DEBUG_MODE          0
#endif

#if APP_DEBUG_MODE == 0
#define DEBUG_ENABLED           true
#else
#define DEBUG_ENABLED           false
#endif

#define DEBUG_SERIAL_BAUD       115200
#define DEBUG_SERIAL_RX         44
#define DEBUG_SERIAL_TX         43

// 🔧 细粒度调试开关（全部关闭以减少性能开销）
#define DEBUG_BATCH_SEND        DEBUG_ENABLED
#define DEBUG_BATCH_DETAIL      DEBUG_ENABLED
#define DEBUG_USB_WRITE         DEBUG_ENABLED
#define DEBUG_CAN_RX            DEBUG_ENABLED
#define DEBUG_CAN_TX            DEBUG_ENABLED
#define DEBUG_QUEUE_STATUS      DEBUG_ENABLED
#define DEBUG_ERROR_ONLY        DEBUG_ENABLED

#if APP_DEBUG_MODE == 1
class DebugNullSerial {
public:
    template<typename... Args> size_t print(Args...) { return 0; }
    template<typename... Args> size_t println(Args...) { return 0; }
    int printf(const char*, ...) { return 0; }
    void begin(unsigned long, uint32_t = SERIAL_8N1, int8_t = -1, int8_t = -1) {}
    operator bool() const { return false; }
};

inline DebugNullSerial& DebugSerialPort() {
    static DebugNullSerial instance;
    return instance;
}

#define Serial0 DebugSerialPort()
#endif

// 便捷宏：简化调试日志写法
#if DEBUG_ENABLED
    #define DEBUG_PRINT(...) Serial0.print(__VA_ARGS__)
    #define DEBUG_PRINTF(...) Serial0.printf(__VA_ARGS__)
    #define DEBUG_PRINTLN(...) Serial0.println(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)
    #define DEBUG_PRINTF(...)
    #define DEBUG_PRINTLN(...)
#endif

// 条件调试（根据开关）
#if DEBUG_ENABLED
    #define DEBUG_IF(cond, ...) do { if (cond) { Serial0.printf(__VA_ARGS__); } } while(0)
#else
    #define DEBUG_IF(cond, ...) do {} while(0)
#endif

// 系统配置
#define CPU_FREQUENCY_MHZ       240
#define DISABLE_WATCHDOG        true

#endif // CONFIG_H
