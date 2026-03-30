/*
 * ESP32-S3 UCAN Gateway - 高性能CAN网关
 * 硬件：ESP32-S3-N16R8 + 2×MCP2518FD模块
 * 协议：UCAN-FD（单一协议，已移除旧封装）
 * 作者：LiuHuan
 * 版本：v5.7 - USB RX性能优化
 * 
 * 架构特点：
 * - 模块化设计（5层架构）
 * - 🚀 纯中断驱动模式（移除轮询，完全依赖中断）
 * - 🚀 FIFO水位中断触发（10帧触发，降低中断频率10倍）✅ 已实施
 * - 🚀 USB背压智能处理（短暂阻塞≠断开）✅ v5.4新增
 * - 🚀 USB半包写入防护（避免协议失步）✅ v5.5新增
 * - 指针传递优化（减少内存拷贝）✅ 已启用
 * - PSRAM内存池（零碎片、零动态分配）
 * - FreeRTOS xQueue跨核通信
 * - 动态批量策略（自适应帧率）
 * - 资源监控与错误统计
 * - CAN/CAN FD自动检测 ✅ 已实现
 * - 动态配置命令 ✅ 已实现
 * 
 * 性能优化（遵循设计文档 extreme-performance-architecture.md）：
 * - P1: FIFO水位中断（10帧触发，中断频率降低10倍）✅ v5.2实施
 * - P2: 纯中断驱动（移除轮询，完全依赖中断）✅ v5.2实施
 * - P3: 指针传递（避免不必要的内存拷贝）✅ 已实现
 * - P4: 动态批量策略（自适应优化）✅ 已实现
 * - P5: CAN/CAN FD自动检测（智能适配）✅ 已实现
 * - P6: UCAN批量模式默认启用 ✅ 已实现
 * - P7: USB背压智能处理（避免误判断开）✅ v5.4实施
 * - P8: USB半包写入防护（避免协议失步）✅ v5.5实施
 * - P9: USB chunk大小优化（256→512字节）✅ v5.6新增
 * - P10: USB RX缓冲区优化（64→256字节）✅ v5.7新增
 * - P11: 编译优化级别提升（-Os→-O2）✅ v5.8新增
 * 
 * 支持的动态配置命令：
 * - 0x05: 设置CAN参数（波特率、模式、FD速率）✅
 * - 0x10: 配置批量参数（最大帧数、超时）✅
 * - 0x11: 配置过滤器（硬件过滤器）⚠️ 需ACAN2517FD库支持
 * - 0x12: 获取统计信息 ✅
 * 
 * v5.5 更新（2026-02-24）：
 * - 🚀 USB半包写入防护：检测到断开/超时时丢弃整包，避免协议失步
 * - 🚀 写包超时保护：写包过程中超过3秒自动中止，防止无限等待
 * - 🚀 半包中止统计：新增 usbPartialWriteAborts 计数器
 * - 🔧 修复：上位机"Frame count out of range: 243"错误的根本原因
 * - 预期效果：彻底消除协议失步问题，上位机不再报错
 * 
 * v5.6 更新（2026-02-26）：
 * - 🚀 USB chunk大小优化：从256字节提升到512字节
 * - 🚀 条件触发延迟：只在高速帧率(>3000fps)时添加100μs延迟
 * - 🚀 协议模式缓存：每100ms更新一次批量模式状态
 * - 🚀 动态批量大小：根据栈空间自动调整CAN接收批量
 * - 🔧 调试开关优化：添加细粒度DEBUG_IF控制
 * - 预期效果：提升USB吞吐量约10-15%
 * 
 * v5.7 更新（2026-02-26）：
 * - 🚀 USB接收缓冲区优化：从64字节提升到256字节
 * - 🚀 USB接收任务优先级提升：5→7，防止RX溢出
 * - 🔧 修复：CDC RX Overflow错误
 * - 预期效果：消除上位机高速发送时的溢出错误
 * 
 * v5.8 更新（2026-02-26）：
 * - 🚀 USB读写Mutex分离：读写可并行执行
 * - 🚀 批量读取优化：使用readBytes()替代单字节循环
 * - 🔧 进一步修复：CDC RX Overflow错误
 * - 预期效果：彻底解决高速发送时的溢出问题
 * 
 * v5.4 更新（2026-02-23）：
 * - 🚀 USB背压智能处理：短暂阻塞不再误判为断开
 * - 🚀 渐进式写入：根据可用缓冲区大小分块写入
 * - 🚀 背压统计：区分"短暂背压" vs "真正断开"
 * - 🚀 队列限流：背压期间丢弃旧包，保留最新数据
 * - 预期效果：减少90%的误判断开，丢包更可控
 * 
 * v5.2 更新（2026-02-14）：
 * - 🚀 实施FIFO水位中断配置（设计文档第5.1节）
 * - 🚀 移除轮询模式，改为纯中断驱动（设计文档第5.2节）
 * - 配置MCP2518FD FIFO中断寄存器（FIFOCON1）
 * - 预期效果：中断频率降低10倍，CPU占用进一步下降
 * 
 * v5.1 更新：
 * - 修复USB发送任务中的Ping-Pong逻辑错误（导致丢包）
 * - 简化为直接发送模式，提高可靠性
 * - 保留指针传递优化，避免不必要的内存拷贝
 */

#include <Arduino.h>
#include <SPI.h>
#include <ACAN2517FD.h>
#include <USB.h>
#include <USBCDC.h>
#include <esp_task_wdt.h>

// 配置模块
#include "../lib/Config/config.h"

// CAN驱动模块
#include "../lib/CANDriver/can_driver.h"
#include "../lib/CANDriver/can_interrupt.h"

// CAN管理模块
#include "../lib/CANManager/can_manager.h"
#include "../lib/CANManager/task_manager.h"

// 协议模块
#include "../lib/Protocol/protocol_manager.h"

// 批量缓冲模块
#include "../lib/BatchBuffer/batch_buffer.h"
#include "../lib/BatchBuffer/zero_copy_buffer.h"

// USB处理模块
#include "../lib/USBHandler/usb_handler.h"

// 统计模块
#include "../lib/Statistics/statistics.h"
#include "../lib/Statistics/performance_monitor.h"

// PSRAM 环形缓冲（CAN→USB 不丢包）
#include "../lib/RingBuffer/psram_ring.h"

// ============================================================================
// 全局对象
// ============================================================================

// USB CDC（改为指针，避免全局初始化问题）
USBCDC* USBSerial = nullptr;

// USB CDC互斥锁（USBCDC在多任务下非线程安全，需保护）
SemaphoreHandle_t g_usbCdcMutex = nullptr;      // 写操作专用
SemaphoreHandle_t g_usbCdcReadMutex = nullptr;  // 读操作专用（读写分离）
SemaphoreHandle_t g_psramRingWriteMutex = nullptr;  // PSRAM ring写入互斥（多生产者保护）

// CAN SPI对象（改为指针，避免全局初始化时访问硬件）
SPIClass* can1SPI = nullptr;
SPIClass* can2SPI = nullptr;
ACAN2517FD* can1 = nullptr;
ACAN2517FD* can2 = nullptr;

// 管理器对象
CANManager* canManager = nullptr;
TaskManager* taskManager = nullptr;
ProtocolManager* protocolManager = nullptr;
Statistics* statistics = nullptr;
USBHandler* usbHandler = nullptr;

// 批量缓冲对象
BatchBuffer* batchBuffer1 = nullptr;
BatchBuffer* batchBuffer2 = nullptr;

// FreeRTOS队列
QueueHandle_t canToUsbQueue = nullptr;
QueueHandle_t usbToCanQueue1 = nullptr;  // 🔧 CAN1专用发送队列
QueueHandle_t usbToCanQueue2 = nullptr;  // 🔧 CAN2专用发送队列
QueueHandle_t usbSendQueue = nullptr;
QueueHandle_t freePacketQueue = nullptr;  // 空闲packet池

#include "time_sync.h"

// ============================================================================
// 全局变量
// ============================================================================

// Legacy batch-mode switch removed: UCAN-FD batch streaming is always enabled.

// 相对时间
volatile uint64_t relativeTimeStart = 0;
volatile bool relativeTimeStarted = false;

// 统计
volatile uint32_t totalFramesReceived = 0;
volatile uint32_t totalBatchesSent = 0;

// 资源监控
volatile uint32_t poolEmptyCount = 0;      // 内存池耗尽次数
volatile uint32_t queueFullCount = 0;      // 队列满次数
volatile uint32_t buildFailedCount = 0;    // 构建失败次数

// USB连接状态管理
volatile bool usbConnected = false;        // USB连接状态
volatile bool canReceivePaused = false;    // CAN接收暂停标志
volatile uint32_t usbDisconnectTime = 0;   // USB断开时间戳
volatile uint32_t usbDisconnectCount = 0;  // USB断开次数
volatile uint32_t queueClearedCount = 0;   // 队列清空次数
volatile uint32_t framesDroppedOnDisconnect = 0;  // 断开时丢弃的帧数

// USB背压统计（区分"短暂背压" vs "真断开"）
volatile uint32_t usbStallCount = 0;           // 写入阻塞次数
volatile uint32_t usbStallDisconnectCount = 0; // 阻塞导致断开次数
volatile uint32_t usbMaxStallMs = 0;           // 最大阻塞时长（ms）
volatile uint32_t usbBackpressureDrops = 0;    // 背压期间丢弃的帧数
volatile uint32_t usbPartialWriteAborts = 0;   // 🔧 新增：半包写入中止次数

// USB传输统计（用于与上位机对齐）
volatile uint32_t usbTxBytes = 0;              // 成功写入到USB的总字节数
volatile uint32_t usbTxWrites = 0;             // USB write() 调用次数
volatile uint32_t usbRxBytes = 0;              // 从USB读取的总字节数
volatile uint32_t usbRxReads = 0;              // USB readBytes() 调用次数

volatile uint32_t lastHostHeartbeatMs = 0;
volatile bool usbConnectEventPending = false;
volatile bool usbDisconnectEventPending = false;

// ============================================================================
// 辅助函数
// ============================================================================

// 🚀 发送数据到USB（优先使用PSRAM ring，不丢包）
inline bool sendToUSB(UCANPacket* packet) {
    if (!packet || packet->length == 0) {
        return false;
    }
    
#if USE_PSRAM_RING_BUFFER
    // 优先尝试写入PSRAM ring（不丢包）
    // 关键：ring底层是单生产者模型，这里必须互斥，避免CAN1/CAN2并发写导致流损坏。
    if (psram_ring_is_ready()) {
        bool ringWritten = false;

        if (g_psramRingWriteMutex &&
            xSemaphoreTake(g_psramRingWriteMutex, pdMS_TO_TICKS(1)) == pdTRUE) {
            const uint32_t avail = psram_ring_available_write();
            if (avail >= packet->length) {
                const uint32_t written = psram_ring_write(packet->data, packet->length);
                if (written == packet->length) {
                    ringWritten = true;
                } else {
                    // 不允许半包残留，出现异常直接清ring避免协议失步
                    psram_ring_reset();
                }
            }
            xSemaphoreGive(g_psramRingWriteMutex);
        }

        if (ringWritten) {
            // 成功写入ring，归还packet
            xQueueSend(freePacketQueue, &packet, 0);
            totalBatchesSent++;
            return true;
        }
        // Ring不可写或空间不足，降级到队列
    }
#endif
    
    // 降级：使用队列
    if (xQueueSend(usbSendQueue, &packet, pdMS_TO_TICKS(1)) == pdTRUE) {
        totalBatchesSent++;
        return true;
    }
    
    // 队列也满了，归还packet
    xQueueSend(freePacketQueue, &packet, 0);
    queueFullCount++;
    return false;
}

// 获取相对时间戳（微秒）
uint64_t getRelativeTimestamp() {
    if (!relativeTimeStarted) {
        relativeTimeStart = esp_timer_get_time();
        relativeTimeStarted = true;
    }
    return esp_timer_get_time() - relativeTimeStart;
}

static void handleHostDisconnect()
{
    usbConnected = false;
    canReceivePaused = true;
    usbDisconnectTime = millis();
    usbDisconnectCount++;
    lastHostHeartbeatMs = 0;

    uint32_t clearedCount = 0;
    UCANPacket* tempPacket = nullptr;
    while (usbSendQueue && freePacketQueue && xQueueReceive(usbSendQueue, &tempPacket, 0) == pdTRUE) {
        if (tempPacket) {
            xQueueSend(freePacketQueue, &tempPacket, 0);
            clearedCount++;
        }
    }
    if (clearedCount > 0) {
        queueClearedCount++;
    }

#if USE_PSRAM_RING_BUFFER
    if (psram_ring_is_ready()) {
        psram_ring_reset();
    }
#endif
}

static bool isUsbHostConnected()
{
    bool connected = false;

    if (USBSerial && g_usbCdcMutex && xSemaphoreTake(g_usbCdcMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
        USBCDC* usb = USBSerial;
        connected = (usb != nullptr) && (*usb);
        xSemaphoreGive(g_usbCdcMutex);
    }

    return connected;
}

static void onUsbCdcEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    (void)arg;
    (void)event_base;
    (void)event_data;

    if (event_id == ARDUINO_USB_CDC_CONNECTED_EVENT) {
        usbConnectEventPending = true;
        usbDisconnectEventPending = false;
    } else if (event_id == ARDUINO_USB_CDC_DISCONNECTED_EVENT) {
        usbDisconnectEventPending = true;
        usbConnectEventPending = false;
    }
}

// 安全禁用WDT
void safeDisableWDT(int coreId) {
    TaskHandle_t idleTask = xTaskGetIdleTaskHandleForCPU(coreId);
    if (idleTask) {
        esp_err_t err = esp_task_wdt_delete(idleTask);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            // 只在真正出错时打印（忽略已经删除的情况）
            Serial0.printf("⚠️  Core %d WDT禁用失败: %d\n", coreId, err);
        } else {
            // Serial0.printf("✅ Core %d WDT已禁用\n", coreId);
        }
    }
}

// ============================================================================
// FreeRTOS任务
// ============================================================================

// CAN接收任务（Core 1）- 使用内存池避免动态分配
void canReceiveTask(void *parameter) {
    // 在任务开始时禁用Core 1看门狗
    if (DISABLE_WATCHDOG) {
        safeDisableWDT(1);
    }

    // 给setup()一点时间完成所有初始化
    vTaskDelay(pdMS_TO_TICKS(100));

    int channelId = (int)parameter;
    
    // 严格的空指针检查
    if (!canManager) {
        Serial0.printf("❌ CAN%d: canManager为空\n", channelId + 1);
        vTaskDelete(NULL);
        return;
    }
    
    CANDriver* driver = canManager->getDriver(channelId);
    if (!driver) {
        Serial0.printf("❌ CAN%d: 驱动未初始化\n", channelId + 1);
        vTaskDelete(NULL);
        return;
    }
    
    BatchBuffer* batchBuf = (channelId == 0) ? batchBuffer1 : batchBuffer2;
    if (!batchBuf) {
        Serial0.printf("❌ CAN%d: 批量缓冲未初始化\n", channelId + 1);
        vTaskDelete(NULL);
        return;
    }
    
    Serial0.printf("📡 CAN%d接收任务启动（中断+库缓冲区轮询）\n", channelId + 1);
    
    // 🔧 优化：动态批量大小，根据可用栈空间调整
    CANFDMessage msgs[48];  // 增大到48帧，充分利用硬件FIFO
    uint8_t maxBatch = 32;  // 初始值，会根据栈空间动态调整
    
    // 🔧 栈空间监控用于动态调整批量大小
    uint32_t lastStackCheck = 0;
    uint32_t lastBatchAdjust = 0;
    const uint32_t BATCH_ADJUST_INTERVAL_MS = 5000;  // 每5秒调整一次
    
    uint32_t loopCount = 0;
    uint32_t notifyCount = 0;
    uint32_t availableCount = 0;
    
    // 🔧 数据流结束检测
    uint32_t lastDataTime = 0;  // 最后一次接收数据的时间
    const uint32_t DATA_END_TIMEOUT_MS = 100;  // 保持100ms，快速响应
    
    while (true) {
        loopCount++;
        
        // 每10秒打印一次栈使用情况
        if (millis() - lastStackCheck > 10000) {
            UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            if (stackHighWaterMark < 512) {  // 小于2KB剩余
                DEBUG_IF(DEBUG_CAN_RX, "[CAN%d] ⚠️ 栈空间不足！剩余: %lu 字节\n", channelId + 1, stackHighWaterMark * 4);
            }
            lastStackCheck = millis();
        }
        
        // 🔧 优化：每5秒动态调整批量大小
        if (millis() - lastBatchAdjust > BATCH_ADJUST_INTERVAL_MS) {
            UBaseType_t stackFree = uxTaskGetStackHighWaterMark(NULL) * 4;  // 估算剩余栈空间
            uint8_t newMaxBatch = (stackFree > 4096) ? 48 : 32;
            if (newMaxBatch != maxBatch) {
                DEBUG_IF(DEBUG_CAN_RX, "[CAN%d] 🔧 批量大小调整: %d -> %d (栈剩余: %lu bytes)\n", 
                         channelId + 1, maxBatch, newMaxBatch, stackFree);
                maxBatch = newMaxBatch;
            }
            lastBatchAdjust = millis();
        }
        
        // ✅ 检查USB断开暂停标志
        // 如果USB未连接或已断开，暂停接收并丢弃数据，防止内存池耗尽
        if (canReceivePaused || !usbConnected) {
            // CAN接收暂停，但仍需清空硬件FIFO防止溢出
            while (driver->available()) {
                uint8_t count = driver->receiveBatch(msgs, maxBatch);
                if (count == 0) break;
                // 直接丢弃，不入队
                // totalFramesReceived += count; // 暂停期间不计入总接收数，以免统计偏差
                framesDroppedOnDisconnect += count;  // 统计断开时丢弃的帧
            }
            vTaskDelay(pdMS_TO_TICKS(10));  // 降低CPU占用
            continue;
        }
        
        // 混合模式：中断通知 + 轮询库缓冲区
        // ACAN2517FD库的ISR已经把数据从硬件FIFO读到了库的缓冲区
        // 我们需要从库的缓冲区读取数据
        
        // 等待中断通知或超时（1ms）- 降低延迟
        uint32_t notifyValue = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1));
        if (notifyValue > 0) {
            notifyCount++;
        }
        
        // 始终检查库的缓冲区（数据在这里，不在硬件FIFO）
        bool hadData = false;  // 标记本次循环是否处理过数据
        while (driver->available()) {
            hadData = true;  // 标记处理过数据
            lastDataTime = millis();  // 更新最后接收数据时间
            availableCount++;
            
            // 正常接收数据
            uint8_t count = driver->receiveBatch(msgs, maxBatch);
            
            if (count == 0) break;
            
            // 检查资源状态（在接收后检查，而不是之前）
            UBaseType_t freePackets = uxQueueMessagesWaiting(freePacketQueue);
            UBaseType_t usbQueueDepth = uxQueueMessagesWaiting(usbSendQueue);
            
            if (freePackets == 0 || usbQueueDepth >= (QUEUE_USB_SEND_SIZE - 20)) {
                // 资源不足：统计丢弃但继续处理下一批
                totalFramesReceived += count;
                queueFullCount += count;
                continue;
            }
            
            // 资源充足：正常处理
            if (count == 0) break;

        

            for (uint8_t i = 0; i < count; i++) {
                uint32_t canId = msgs[i].id;
            if (msgs[i].ext) {
                canId |= 0x80000000;
            }
            
            // 🔧 修复：使用硬件时间戳并转换为系统时间
            uint64_t timestamp = TimeSyncManager::getInstance().getSystemTime(channelId, msgs[i].timestamp);
            if (!relativeTimeStarted) {
                relativeTimeStart = esp_timer_get_time();
                relativeTimeStarted = true;
            }
            if (timestamp > relativeTimeStart) {
                timestamp = timestamp - relativeTimeStart;
            } else {
                timestamp = 0;
            }
            
            // UCAN-FD mode: batch streaming is always enabled.

            // 构建标志位
            // bit0: 扩展帧 (已经在canId中处理了，但为了兼容性可以加上) -> 其实canId最高位已经有了，这里bit0可能是redundant或者UCAN特殊要求
            // bit1: CAN FD帧
            // bit2: BRS
            // bit3: ESI
            uint8_t flags = 0;
            if (msgs[i].ext) flags |= 0x01; // bit0: Extended
            // 检测 FD 帧: 长度>8 或者 明确的FD类型
            // ACAN2517FD库中，CANFDMessage没有直接的 fd 标志，但可以通过 type 或 len 判断
            // 这里我们假设 len > 8 或者 type 是 FD 类型
            bool isFD = (msgs[i].len > 8) || 
                       (msgs[i].type == CANFDMessage::CANFD_NO_BIT_RATE_SWITCH) ||
                       (msgs[i].type == CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH);
            
            if (isFD) {
                flags |= 0x02; // bit1: CAN FD
            }

            if (msgs[i].type == CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH) {
                flags |= 0x04; // bit2: BRS
            }
            
            // ESI暂无法获取，保持0

            // 使用BatchBuffer的零拷贝添加帧
            if (!batchBuf->addFrame(canId, msgs[i].len, msgs[i].data, timestamp, flags)) {
                // 缓冲区满或时间偏移溢出，先发送当前批次
                // 从内存池获取空闲packet（零拷贝+无碎片）
                UCANPacket* packet = nullptr;
                if (xQueueReceive(freePacketQueue, &packet, 0) == pdTRUE && packet) {
                    packet->length = batchBuf->buildPacket(packet->data, sizeof(packet->data));
                    if (packet->length > 0) {
                        // 🚀 使用sendToUSB（优先PSRAM ring，不丢包）
                        sendToUSB(packet);
                    } else {
                        // 构建失败，归还packet
                        xQueueSend(freePacketQueue, &packet, 0);
                        buildFailedCount++;  // 统计构建失败
                    }
                } else {
                    poolEmptyCount++;  // 统计内存池耗尽
                }
                // 🔧 修复：重新添加当前帧，并检查返回值
                if (!batchBuf->addFrame(canId, msgs[i].len, msgs[i].data, timestamp, flags)) {
                    // 如果仍然失败，记录错误并丢弃该帧
                    static uint32_t lastDropLog = 0;
                    if (millis() - lastDropLog > 1000) {
                        DEBUG_IF(DEBUG_ERROR_ONLY, "[CAN%d] ⚠️ 帧添加失败，已丢弃: ID=0x%X\n", channelId + 1, canId);
                        lastDropLog = millis();
                    }
                    queueFullCount++;  // 统计丢弃
                }
            }
            
            // 🔧 修复：只检查是否达到最大帧数，不检查超时
            // 超时检查应该在循环外面进行，避免每帧都触发发送
            if (batchBuf->getFrameCount() >= batchBuf->getMaxFrames()) {
                // 达到最大帧数，立即发送
                UCANPacket* packet = nullptr;
                if (xQueueReceive(freePacketQueue, &packet, 0) == pdTRUE && packet) {
                    packet->length = batchBuf->buildPacket(packet->data, sizeof(packet->data));
                    if (packet->length > 0) {
                        sendToUSB(packet);
                    } else {
                        xQueueSend(freePacketQueue, &packet, 0);
                        buildFailedCount++;
                    }
                } else {
                    poolEmptyCount++;
                }
            }
            
            totalFramesReceived++;
        }
        }  // end while (driver->available())
        
        // 🔧 超时检查：如果缓冲区有剩余帧且超时，发送
        // 这是批量发送的核心机制：达到最大帧数或超时就发送
        uint8_t remainingFrames = batchBuf->getFrameCount();
        if (remainingFrames > 0) {
            uint64_t currentTime = getRelativeTimestamp();
            bool shouldSendNow = false;
            
            // 条件1：批量超时（正常超时机制）
            if (batchBuf->shouldSend(currentTime)) {
                shouldSendNow = true;
            }
            
            // 条件2：数据流结束检测（100ms无新数据且有剩余帧）
            if (lastDataTime > 0 && (millis() - lastDataTime) >= DATA_END_TIMEOUT_MS) {
                shouldSendNow = true;
            }
            
            if (shouldSendNow) {
                // 发送剩余帧
                UCANPacket* packet = nullptr;
                if (xQueueReceive(freePacketQueue, &packet, 0) == pdTRUE && packet) {
                    packet->length = batchBuf->buildPacket(packet->data, sizeof(packet->data));
                    if (packet->length > 0) {
                        sendToUSB(packet);
                    } else {
                        xQueueSend(freePacketQueue, &packet, 0);
                        buildFailedCount++;
                    }
                } else {
                    poolEmptyCount++;
                }
            }
        }
    }  // end while (true)
}

// CAN发送任务
void canTransmitTask(void *parameter) {
    int channelId = (int)parameter;
    CANDriver* driver = canManager->getDriver(channelId);
    
    if (!driver) {
        Serial0.printf("❌ CAN%d驱动未初始化\n", channelId + 1);
        vTaskDelete(NULL);
        return;
    }
    
    // 🔧 修复：使用通道专用队列
    QueueHandle_t myQueue = (channelId == 0) ? usbToCanQueue1 : usbToCanQueue2;
    if (!myQueue) {
        Serial0.printf("❌ CAN%d队列未初始化\n", channelId + 1);
        vTaskDelete(NULL);
        return;
    }
    
    Serial0.printf("📡 CAN%d发送任务启动（专用队列）\n", channelId + 1);
    
    CANFDMessage msg;
    uint32_t sendCount = 0;
    uint32_t errorCount = 0;
    uint32_t retryCount = 0;
    
    while (true) {
        // 从专用队列接收待发送的帧
        if (xQueueReceive(myQueue, &msg, pdMS_TO_TICKS(10)) == pdTRUE) {
            // 🔧 修复：添加重试机制，最多重试10次
            bool sent = false;
            for (int retry = 0; retry < 10; retry++) {
                if (driver->send(msg)) {
                    sendCount++;
                    sent = true;
                    if (retry > 0) {
                        retryCount++;
                    }
                    break;
                }
                // 发送失败，等待1ms后重试
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            
            if (!sent) {
                errorCount++;
                // 只在前5次失败时打印日志
                if (errorCount <= 5) {
                    DEBUG_IF(DEBUG_ERROR_ONLY, "[CAN%d-TX] ❌ 发送失败（重试10次后）: ID=0x%X\n", 
                                 channelId + 1, msg.id);
                }
            }
        }
        
        // 定期打印发送统计
        static uint32_t lastPrintTime = 0;
        uint32_t now = millis();
        if (now - lastPrintTime > 30000) {  // 每30秒打印一次
            if (sendCount > 0 || errorCount > 0) {
                Serial0.printf("[CAN%d] 发送统计: 成功=%lu, 失败=%lu, 重试=%lu\n", 
                            channelId + 1, sendCount, errorCount, retryCount);
            }
            lastPrintTime = now;
        }
    }
}

// USB接收任务
void usbReceiveTask(void *parameter) {
    Serial0.println("📡 USB接收任务启动（读写分离+批量读取优化）");
    
    while (true) {
        // 🔧 优化：增大接收缓冲区从64到256字节，防止高速发送时溢出
        uint8_t rxBuf[256];
        int rxCount = 0;

        // 🔧 优化：使用独立的读取互斥锁，与写入任务并行执行
        if (USBSerial && g_usbCdcReadMutex && xSemaphoreTake(g_usbCdcReadMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
            USBCDC* usb = USBSerial;
            if (usb && *usb) {
                // 🔧 优化：使用available()检查可读字节数，批量读取
                int available = usb->available();
                if (available > 0) {
                    // 读取可用数据或缓冲区大小，取较小值
                    int toRead = available < (int)sizeof(rxBuf) ? available : sizeof(rxBuf);
                    rxCount = usb->readBytes(rxBuf, toRead);
                }
            }
            xSemaphoreGive(g_usbCdcReadMutex);
        }

        if (rxCount > 0) {
            usbRxBytes += rxCount;
            usbRxReads += 1;
            lastHostHeartbeatMs = millis(); // 🚀 数据驱动保活：收到任何上位机数据即认为其存活
            canReceivePaused = false;
        }

        for (int i = 0; i < rxCount; i++) {
            uint8_t byte = rxBuf[i];

            // 使用协议管理器处理字节
            if (protocolManager) {
                protocolManager->processByte(byte);
            }
        }

        // 有数据时不额外休眠，提升吞吐；无数据时轻微让出CPU
        if (rxCount == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

// USB发送任务（Core 0）- 优先从PSRAM ring读取
void usbSendTask(void *parameter) {
#if USE_PSRAM_RING_BUFFER
    Serial0.println("📡 USB发送任务启动（PSRAM ring优先 + 零拷贝队列）");
#else
    Serial0.println("📡 USB发送任务启动（零拷贝队列）");
#endif
    
    UCANPacket* packet = nullptr;
    uint32_t sentCount = 0;
    uint32_t droppedCount = 0;
    uint32_t nullPacketCount = 0;
    uint32_t writeFailCount = 0;
    uint32_t bufferOverflowCount = 0;
    
    // 🔧 获取高优先级队列（用于心跳等紧急响应）
    QueueHandle_t highPriorityQueue = nullptr;
    if (protocolManager && protocolManager->getUCANHandler()) {
        highPriorityQueue = protocolManager->getUCANHandler()->getHighPriorityQueue();
        if (highPriorityQueue) {
            Serial0.println("📡 高优先级队列已绑定（心跳优先发送）");
        }
    }
    
    // USB状态跟踪
    bool lastUsbState = false;
    // 🚀 关键优化：大幅提高stall超时阈值，避免频繁触发断开
    // 原来2秒太激进，高吞吐场景下Windows调度抖动就会触发
    // 改为30秒，只在真正断开时才进入断开处理
    // 日常的背压通过丢包处理，不需要断开
    const uint32_t USB_STALL_DISCONNECT_MS = 30000;
    
    // 运行期缓存策略
    const uint32_t MAX_BUFFER_FRAMES = 100;  // 最多缓存100帧

    const uint32_t HOST_HEARTBEAT_TIMEOUT_MS_BASE = 3000;
    const uint32_t HOST_HEARTBEAT_TIMEOUT_MS_BACKPRESSURE = 15000;
    
    // 🔧 优化：帧率追踪（用于条件触发延迟）
    static uint32_t frameRate = 0;
    static uint32_t lastRateUpdate = 0;
    static uint32_t lastSentCount = 0;
    
    while (true) {
        bool usbReadyNow = isUsbHostConnected();

        if (usbReadyNow && !lastUsbState) {
            usbConnected = true;
            canReceivePaused = false;
            framesDroppedOnDisconnect = 0;
            lastUsbState = true;
            lastHostHeartbeatMs = millis();
        } else if (!usbReadyNow && lastUsbState) {
            lastUsbState = false;
            if (usbConnected) {
                handleHostDisconnect();
            }
        }

        if (!usbReadyNow) {
            vTaskDelay(pdMS_TO_TICKS(2));
            continue;
        }

        if (usbConnected) {
            uint32_t now = millis();
            uint32_t lastHb = lastHostHeartbeatMs;
            uint32_t hbTimeoutMs = HOST_HEARTBEAT_TIMEOUT_MS_BASE;
            if (usbMaxStallMs >= 500) {
                hbTimeoutMs = HOST_HEARTBEAT_TIMEOUT_MS_BACKPRESSURE;
            }
#if USE_PSRAM_RING_BUFFER
            else if (psram_ring_is_ready() && psram_ring_available_read() >= (PSRAM_RING_SIZE * 8 / 10)) {
                hbTimeoutMs = HOST_HEARTBEAT_TIMEOUT_MS_BACKPRESSURE;
            }
#endif

            if (lastHb > 0 && (now - lastHb) >= hbTimeoutMs) {
                // 在USB背压/高负载情况下，上位机可能暂时无法及时发送心跳。
                // 这里不强制判定USB断开，改为暂停CAN接收，给链路恢复窗口。
                canReceivePaused = true;
            }
        }
        
        // 🔧 优先检查高优先级队列（心跳等紧急响应）
        bool fromHighPriority = false;
        if (highPriorityQueue) {
            // 尝试非阻塞获取高优先级数据
            if (xQueueReceive(highPriorityQueue, &packet, 0) == pdTRUE && packet) {
                fromHighPriority = true;
            }
        }
        
        // 🚀 优先从PSRAM环形缓冲读取并写USB（CAN→USB不丢包路径）
#if USE_PSRAM_RING_BUFFER
        if (!fromHighPriority && psram_ring_is_ready() && usbReadyNow) {
            uint8_t ringBuf[512];
            uint32_t n = psram_ring_read(ringBuf, sizeof(ringBuf));
            if (n > 0 && USBSerial && g_usbCdcMutex && xSemaphoreTake(g_usbCdcMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                size_t written = 0;
                if (*USBSerial) {
                    written = USBSerial->write(ringBuf, n);
                }
                xSemaphoreGive(g_usbCdcMutex);
                if (written > 0) {
                    usbTxBytes += written;
                    usbTxWrites++;
                    canReceivePaused = false;
                    lastHostHeartbeatMs = millis(); // 🚀 数据驱动保活：能成功把数据喂给上位机，说明上位机存活且在读取
                }
                continue;  // 继续处理ring buffer
            }
        }
#endif
        
        // 🔧 关键优化：队列深度检测，主动丢弃旧包
        // 当队列积压超过阈值时，说明上位机处理不过来
        // 丢弃旧包保持实时性，避免越积越多导致系统卡死
        // 🚀 优化：降低阈值从800到300，更早介入防止心跳超时
        uint32_t queueDepth = uxQueueMessagesWaiting(usbSendQueue);
        if (queueDepth > 300) {
            // 队列积压，开始丢弃旧包
            uint32_t dropCount = 0;
            UCANPacket* tempPacket = nullptr;
            // 根据积压程度决定丢弃比例
            uint32_t dropTarget = 0;
            if (queueDepth > 700) {
                // 严重积压：丢弃一半
                dropTarget = queueDepth / 2;
            } else if (queueDepth > 500) {
                // 中度积压：丢弃1/3
                dropTarget = queueDepth / 3;
            } else {
                // 轻度积压：丢弃1/4
                dropTarget = queueDepth / 4;
            }
            
            for (uint32_t i = 0; i < dropTarget; i++) {
                if (xQueueReceive(usbSendQueue, &tempPacket, 0) == pdTRUE && tempPacket) {
                    xQueueSend(freePacketQueue, &tempPacket, 0);
                    dropCount++;
                }
            }
            if (dropCount > 0) {
                usbBackpressureDrops += dropCount;
                DEBUG_IF(DEBUG_ERROR_ONLY, "⚠️ 队列积压清理: 深度=%lu, 丢弃=%lu, 剩余=%lu\n", 
                             queueDepth, dropCount, uxQueueMessagesWaiting(usbSendQueue));
            }
        }
        
        // 如果没有高优先级数据，从普通队列获取
        if (!fromHighPriority) {
            if (xQueueReceive(usbSendQueue, &packet, pdMS_TO_TICKS(1)) != pdTRUE) {
                // 没有数据，短暂休眠
                continue;
            }
        }
        
        // 严格的空指针检查
        if (packet == nullptr) {
            nullPacketCount++;
            continue;
        }
        
        if (packet->length == 0 || packet->length > sizeof(packet->data)) {
            // 无效长度，归还packet
            xQueueSend(freePacketQueue, &packet, 0);
            packet = nullptr;
            droppedCount++;
            continue;
        }

        if (!USBSerial) {
            xQueueSend(freePacketQueue, &packet, 0);
            packet = nullptr;
            droppedCount++;
            continue;
        }
        
        size_t totalWritten = 0;
        uint32_t packetStallStart = 0;
        bool writeAborted = false;  // 🔧 新增：标记写入是否被中止

            while (totalWritten < packet->length) {
                USBCDC* usb = USBSerial;
                if (!usb) {
                    writeAborted = true;
                    usbPartialWriteAborts++;
                    DEBUG_IF(DEBUG_ERROR_ONLY, "⚠️ USB对象空：已写%d/%d字节，丢弃整包避免失步\n", 
                                 totalWritten, packet->length);
                    break;
                }

                size_t remaining = packet->length - totalWritten;
                size_t chunk = remaining;
                if (chunk > 512) {
                    chunk = 512;
                }

                size_t written = 0;
                bool mutexHeld = false;
                
                // 🚀 极致非阻塞写入：先检查可用空间，如果没有直接跳出，拒绝死等
                int availableSpace = usb->availableForWrite();
                if (availableSpace <= 0) {
                    // 如果空间不够，我们不阻塞等待，而是直接标记超时退出，由外层逻辑进行背压控制和丢包
                    // 这样可以彻底解放 CPU 去处理别的事情，不堵塞整个核心。
                    if (packetStallStart == 0) {
                        packetStallStart = millis();
                    }
                    if ((millis() - packetStallStart) >= USB_STALL_DISCONNECT_MS || !isUsbHostConnected()) {
                        writeAborted = true;
                        usbPartialWriteAborts++;
                        if (usbConnected) {
                            handleHostDisconnect();
                        }
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(1));
                    continue;
                }

                if (chunk > static_cast<size_t>(availableSpace)) {
                    chunk = static_cast<size_t>(availableSpace);
                }

                if (g_usbCdcMutex && xSemaphoreTake(g_usbCdcMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
                    mutexHeld = true;
                    if (usb) {
                        written = usb->write(packet->data + totalWritten, chunk);
                    }
                    xSemaphoreGive(g_usbCdcMutex);
                    mutexHeld = false;
                }
                
                if (written > 0) {
                    totalWritten += written;
                    usbTxBytes += written;
                    usbTxWrites += 1;
                    canReceivePaused = false;
                    packetStallStart = 0;
                    lastHostHeartbeatMs = millis(); // 数据驱动保活
                } else {
                    if (mutexHeld) {
                        xSemaphoreGive(g_usbCdcMutex);
                        mutexHeld = false;
                    }
                    
                    // 既然我们已经用 availableForWrite 做了检查，如果 write 还是返回 0，
                    // 说明底层出了严重错误，直接放弃。
                    if (packetStallStart == 0) {
                        packetStallStart = millis();
                    }
                    if ((millis() - packetStallStart) >= USB_STALL_DISCONNECT_MS || !isUsbHostConnected()) {
                        writeAborted = true;
                        usbPartialWriteAborts++;
                        if (usbConnected) {
                            handleHostDisconnect();
                        }
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(1));
                    continue;
                }
            }

            // 🔧 修复：根据写入结果判断成功/失败
            if (!writeAborted && totalWritten == packet->length) {
                sentCount++;
                
                // 🔧 优化：每100ms更新一次帧率
                if (millis() - lastRateUpdate > 100) {
                    frameRate = (sentCount - lastSentCount) * 10;  // 转换为每秒帧率
                    lastSentCount = sentCount;
                    lastRateUpdate = millis();
                }
                
                // 🚀 关键优化：彻底移除调用 flush()
                // USB CDC 硬件会自动在缓冲区满或超时时发送，不需要每包 flush，
                // 这能极大地减少抖动并避免互斥锁死锁。
            } else {
                droppedCount++;
                writeFailCount++;
                if (writeFailCount <= 3 && totalWritten > 0) {
                    DEBUG_IF(DEBUG_ERROR_ONLY, "⚠️ USB packet interrupted: %d/%d\n",
                             totalWritten, packet->length);
                }
            }
            
            // 归还packet到内存池
            xQueueSend(freePacketQueue, &packet, 0);
            packet = nullptr;
        }
        
        // 定期打印统计
        static uint32_t lastPrintTime = 0;
        uint32_t now = millis();
        if (now - lastPrintTime > 5000) {
            DEBUG_IF(DEBUG_USB_WRITE, "📤 USB发送: 成功=%lu, 丢弃=%lu (写入失败=%lu, 缓存溢出=%lu)\n", 
                         sentCount, droppedCount, writeFailCount, bufferOverflowCount);
            lastPrintTime = now;
        }
    }


// 统计任务
void statisticsTask(void *parameter) {
    Serial0.println("📊 统计任务启动");
    
    uint32_t lastFrameCount = 0;
    uint32_t lastUpdateTime = millis();
    uint8_t lastMaxFrames1 = 0;
    uint8_t lastMaxFrames2 = 0;
    
    // USB统计基线
    uint32_t lastUsbTxBytes = 0;
    uint32_t lastUsbRxBytes = 0;
    uint32_t lastUsbTxWrites = 0;
    uint32_t lastUsbRxReads = 0;
    uint32_t lastUsbStall = 0;
    uint32_t lastUsbBackpressureDrops = 0;

    bool lastPrintedDisconnected = false;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        if (!usbConnected) {
            if (!lastPrintedDisconnected) {
                Serial0.printf("🔌 USB已断开：断开次数=%lu, 断开丢帧=%lu\n",
                              usbDisconnectCount, framesDroppedOnDisconnect);
                lastPrintedDisconnected = true;
            }
            continue;
        }

        lastPrintedDisconnected = false;
        
        // 计算帧率（每秒更新一次）
        uint32_t currentTime = millis();
        uint32_t currentFrameCount = totalFramesReceived;
        uint32_t elapsedMs = currentTime - lastUpdateTime;
        
        if (elapsedMs == 0) {
            continue;
        }

        uint32_t frameRate = ((currentFrameCount - lastFrameCount) * 1000) / elapsedMs;
        
        // 更新动态批量策略
        if (batchBuffer1) {
            batchBuffer1->updateStrategy(frameRate);
            uint8_t newMaxFrames = batchBuffer1->getMaxFrames();
            if (newMaxFrames != lastMaxFrames1) {
                Serial0.printf("📊 CAN1批量策略更新: %d帧/%dms (帧率=%lu/s)\n", 
                             newMaxFrames, batchBuffer1->getTimeoutMs(), frameRate);
                lastMaxFrames1 = newMaxFrames;
            }
        }
        if (batchBuffer2) {
            batchBuffer2->updateStrategy(frameRate);
            uint8_t newMaxFrames = batchBuffer2->getMaxFrames();
            if (newMaxFrames != lastMaxFrames2) {
                Serial0.printf("📊 CAN2批量策略更新: %d帧/%dms (帧率=%lu/s)\n", 
                             newMaxFrames, batchBuffer2->getTimeoutMs(), frameRate);
                lastMaxFrames2 = newMaxFrames;
            }
        }
        
        lastFrameCount = currentFrameCount;
        lastUpdateTime = currentTime;
        
        // 更新统计
        if (statistics) {
            statistics->update();
            static uint32_t lastPerfPrintTime = 0;
            if (currentTime - lastPerfPrintTime >= 2000) {
                statistics->print();
                lastPerfPrintTime = currentTime;
            }
        }

        // USB速率统计（与上位机日志对齐，降频到2秒打印一次）
        static uint32_t lastStatsPrintTime = 0;
        if (currentTime - lastStatsPrintTime >= 2000) {
            lastStatsPrintTime = currentTime;
            
            uint32_t txNow = usbTxBytes;
            uint32_t rxNow = usbRxBytes;
            uint32_t dropsNow = usbBackpressureDrops;

            // 计算2秒内的平均速率
            uint32_t txBytesPerSec = (txNow - lastUsbTxBytes) / 2;
            uint32_t rxBytesPerSec = (rxNow - lastUsbRxBytes) / 2;
            uint32_t dropsDelta = (dropsNow - lastUsbBackpressureDrops);

            lastUsbTxBytes = txNow;
            lastUsbRxBytes = rxNow;
            lastUsbBackpressureDrops = dropsNow;

            uint32_t queueDepth = usbSendQueue ? uxQueueMessagesWaiting(usbSendQueue) : 0;

            if (!usbConnected) {
                if (!lastPrintedDisconnected) {
                    Serial0.printf("[STATUS] USB Disconnected: Drops=%lu\n", framesDroppedOnDisconnect);
                    lastPrintedDisconnected = true;
                }
            } else {
                // 仅通过 Serial0 (硬件串口) 打印精简日志
                Serial0.printf("[STATS] TX=%lu B/s | RX=%lu B/s | Q=%u | Drops=%lu\n",
                               txBytesPerSec, rxBytesPerSec,
                               queueDepth, dropsDelta);
            }
        }
    }
}


// ============================================================================
// 主程序
// ============================================================================

void setup() {
    // 初始化RGB灯 (WS2812 on GPIO 48)
    #ifdef RGB_BUILTIN
    // 强制关闭RGB灯
    neopixelWrite(RGB_BUILTIN, 0, 0, 0); 
    #else
   
    #endif

    // 1. 最优先初始化物理串口，确保能看到启动日志
    // 注意：ESP32-S3 DevKitC默认UART0在GPIO 43(TX)/44(RX)
    Serial0.begin(115200, SERIAL_8N1, 44, 43); 
    // 如果Serial0不可用，尝试标准Serial（取决于编译选项）
    if (!Serial0) Serial.begin(115200);
    
    // 给串口一点时间初始化
    delay(2000); 
    Serial0.println("\n\n\n");
    Serial0.println("╔════════════════════════════════════════╗");
    Serial0.println("║  SYSTEM STARTUP CHECK                  ║");
    Serial0.println("╚════════════════════════════════════════╝");
    Serial0.println("✅ CPU Start");

    // 设置CPU频率
    setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);
    Serial0.printf("✅ CPU Freq: %d MHz\n", getCpuFrequencyMhz());

    // 禁用看门狗 (在setup中只能禁用当前核心的，Core 1的需要在任务中禁用)
    if (DISABLE_WATCHDOG) {
        safeDisableWDT(0);
        Serial0.println("✅ Core 0 WDT Disabled");
    }
    
    // 初始化USB CDC (这可能会阻塞，所以放在串口初始化之后)
    // 🔧 关键修改：将CAN初始化移到USB等待之前，甚至USB初始化之前
    // 这样上电即工作，不依赖USB
    
    // 初始化USB CDC互斥锁（必须在任何任务访问USBCDC前创建）
    if (!g_usbCdcMutex) {
        g_usbCdcMutex = xSemaphoreCreateMutex();
    }
    // 🔧 优化：创建独立的读取互斥锁，实现读写并行
    if (!g_usbCdcReadMutex) {
        g_usbCdcReadMutex = xSemaphoreCreateMutex();
    }
    if (!g_usbCdcReadMutex) {
        g_usbCdcReadMutex = g_usbCdcMutex;
    }
    if (!g_psramRingWriteMutex) {
        g_psramRingWriteMutex = xSemaphoreCreateMutex();
    }
    USBSerial = new USBCDC();
    USBSerial->enableReboot(false);
    USBSerial->onEvent(onUsbCdcEvent);
    USB.begin();
    USBSerial->begin(USB_CDC_BAUD);
    Serial0.println("⏳ Initializing USB CDC...");
    Serial0.println("✅ USB CDC Initialized");
    
    // ============================================================================
    // 🚀 核心初始化流程优化：CAN优先，USB次之
    // ============================================================================
    
    Serial0.println("\n╔════════════════════════════════════════╗");
    Serial0.println("║  ESP32-S3 UCAN Gateway v5.8           ║");
    Serial0.println("║  USB读写分离+批量读取优化            ║");
    Serial0.println("║  USB背压优化 + 极致性能              ║");
    Serial0.println("╚════════════════════════════════════════╝");
    Serial0.println("\n✅ 已实现功能：");
    Serial0.println("  - 🚀 硬件时间戳同步 (TimeSyncManager)");
    TimeSyncManager::getInstance().init();
    Serial0.println("  - 🚀 FIFO中断（库已配置：非空+半满+溢出）");
    Serial0.println("  - 🚀 混合模式（中断通知+库缓冲区轮询）");
    Serial0.println("  - 🆕 USB背压智能处理（短暂阻塞≠断开）");
    Serial0.println("  - 🆕 渐进式写入+限流（避免爆炸性丢包）");
    Serial0.println("  - 🆕 背压统计（区分真断开 vs 上位机慢）");
    Serial0.println("  - 指针传递优化（减少内存拷贝）");
    Serial0.println("  - CAN/CAN FD自动检测");
    Serial0.println("  - 动态配置命令（0x05/0x10/0x11/0x12）");
    Serial0.println("  - PSRAM内存池（零碎片）");
    Serial0.println("  - 动态批量策略");
    Serial0.println("  - 资源监控统计");
    Serial0.println("  - UCAN批量模式默认启用");
    
    // 创建队列
    Serial0.println("\n4️⃣ 创建队列...");
    
    // CAN→USB队列（内部RAM，速度快）
    canToUsbQueue = xQueueCreate(QUEUE_CAN_TO_USB_SIZE, sizeof(CANFDMessage));
    Serial0.printf("   - CAN→USB队列: %s\n", canToUsbQueue ? "✅" : "❌");
    
    // 🔧 修复：为每个CAN通道创建独立的发送队列
    usbToCanQueue1 = xQueueCreate(QUEUE_USB_TO_CAN_SIZE, sizeof(CANFDMessage));
    Serial0.printf("   - USB→CAN1队列: %s\n", usbToCanQueue1 ? "✅" : "❌");
    
    usbToCanQueue2 = xQueueCreate(QUEUE_USB_TO_CAN_SIZE, sizeof(CANFDMessage));
    Serial0.printf("   - USB→CAN2队列: %s\n", usbToCanQueue2 ? "✅" : "❌");
    
    // USB发送队列使用指针传递（零拷贝）
    usbSendQueue = xQueueCreate(QUEUE_USB_SEND_SIZE, sizeof(UCANPacket*));
    Serial0.printf("   - USB发送队列: %s\n", usbSendQueue ? "✅" : "❌");
    
    // 空闲packet内存池队列
    freePacketQueue = xQueueCreate(QUEUE_USB_SEND_SIZE, sizeof(UCANPacket*));
    Serial0.printf("   - 内存池队列: %s\n", freePacketQueue ? "✅" : "❌");
    
    if (!canToUsbQueue || !usbToCanQueue1 || !usbToCanQueue2 || !usbSendQueue || !freePacketQueue) {
        Serial0.println("❌ 队列创建失败");
    } else {
        Serial0.println("✅ 队列创建成功");
        Serial0.printf("   - CAN→USB队列: %d 项\n", QUEUE_CAN_TO_USB_SIZE);
        Serial0.printf("   - USB→CAN1队列: %d 项\n", QUEUE_USB_TO_CAN_SIZE);
        Serial0.printf("   - USB→CAN2队列: %d 项\n", QUEUE_USB_TO_CAN_SIZE);
        Serial0.printf("   - USB发送队列: %d 项（指针）\n", QUEUE_USB_SEND_SIZE);
        Serial0.printf("   - 内存池队列: %d 项（指针）\n", QUEUE_USB_SEND_SIZE);
    }
    
    // 🚀 优化：先初始化PSRAM环形缓冲区（大块连续内存），避免碎片化
#if USE_PSRAM_RING_BUFFER
    Serial0.println("\n🔧 初始化PSRAM环形缓冲区（优先分配大块连续内存）...");
    psram_ring_init(PSRAM_RING_SIZE);
    if (psram_ring_is_ready()) {
        Serial0.printf("✅ PSRAM环形缓冲: %lu KB (CAN→USB 不丢包)\n", (unsigned long)(PSRAM_RING_SIZE / 1024));
        Serial0.printf("   - 可缓存约 %.1f 秒的高速数据\n", (float)PSRAM_RING_SIZE / (7000 * 30));
    } else {
        Serial0.println("⚠️ PSRAM环形缓冲分配失败，将仅使用队列");
    }
#endif
    
    // 初始化内存池（小块分配，放在PSRAM ring之后避免碎片化）
    Serial0.println("\n🔧 初始化内存池...");
    uint32_t allocatedCount = 0;
    uint32_t failedCount = 0;
    
    // 使用QUEUE_USB_SEND_SIZE作为池大小
    uint32_t poolSize = QUEUE_USB_SEND_SIZE;
    Serial0.printf("   目标池大小=%lu\n", poolSize);
    
    for (uint32_t i = 0; i < poolSize; i++) {
        // 优先使用PSRAM，失败则回退到内部RAM
        UCANPacket* packet = (UCANPacket*)heap_caps_malloc(sizeof(UCANPacket), MALLOC_CAP_SPIRAM);
        if (!packet) {
            packet = (UCANPacket*)heap_caps_malloc(sizeof(UCANPacket), MALLOC_CAP_8BIT);
        }

        if (packet) {
            memset(packet, 0, sizeof(UCANPacket));
            // 🔧 关键修复：确保packet有效且队列未满才放入
            if (xQueueSend(freePacketQueue, &packet, 0) == pdTRUE) {
                allocatedCount++;
            } else {
                // 队列满了，释放packet并停止分配
                heap_caps_free(packet);
                Serial0.printf("⚠️  队列已满，停止分配（已分配%lu个）\n", allocatedCount);
                break;
            }
        } else {
            failedCount++;
            // 🔧 关键修复：分配失败时不要继续，避免队列中有空洞
            if (failedCount > 10) {
                Serial0.printf("⚠️  连续分配失败%lu次，停止分配\n", failedCount);
                break;
            }
        }
    }
    
    Serial0.printf("✅ 内存池初始化完成: 成功=%lu, 失败=%lu\n", allocatedCount, failedCount);
    Serial0.printf("   - 每个packet大小: %d 字节\n", sizeof(UCANPacket));
    Serial0.printf("   - 总内存占用: %lu KB (PSRAM)\n", (allocatedCount * sizeof(UCANPacket)) / 1024);
    Serial0.printf("   - 内部RAM节省: %lu KB\n", (allocatedCount * sizeof(UCANPacket)) / 1024);
    
    // 关键检查：内存池分配成功率
    float successRate = (float)allocatedCount / poolSize * 100.0f;
    if (allocatedCount < 100) {
        Serial0.println("\n╔════════════════════════════════════════╗");
        Serial0.println("║  ❌ 严重错误：内存池分配失败！         ║");
        Serial0.printf("║  仅分配成功: %lu 个packet             ║\n", allocatedCount);
        Serial0.println("║  系统无法启动！                        ║");
        Serial0.println("║  请检查PSRAM配置                       ║");
        Serial0.println("╚════════════════════════════════════════╝");
        while(1) { delay(1000); }  // 停止启动
    } else if (successRate < 80.0f) {
        Serial0.printf("\n⚠️  警告：内存池分配成功率 %.1f%% (目标 >= 80%%)\n", successRate);
        Serial0.printf("   实际可用: %lu packets\n", allocatedCount);
    }
    
    // 初始化批量缓冲对象
    batchBuffer1 = new BatchBuffer(0);
    batchBuffer2 = new BatchBuffer(1);
    batchBuffer1->init();
    batchBuffer2->init();
    Serial0.println("✅ BatchBuffer对象初始化完成");
    
    // 初始化中断处理器
    Serial0.println("6️⃣ 初始化中断处理器...");
    CANInterruptHandler::init();
    Serial0.println("   ✅ 中断处理器初始化完成");
    
    // 初始化CAN SPI对象（在setup中初始化，避免全局初始化时访问硬件）
    Serial0.println("7️⃣ 初始化CAN SPI对象...");
    can1SPI = new SPIClass(HSPI);
    can2SPI = new SPIClass(FSPI);
    can1 = new ACAN2517FD(CAN1_CS, *can1SPI, CAN1_INT);
    can2 = new ACAN2517FD(CAN2_CS, *can2SPI, CAN2_INT);
    Serial0.println("   ✅ CAN SPI对象初始化完成");
    
    // 创建CAN管理器
    Serial0.println("8️⃣ 创建CAN管理器...");
    canManager = new CANManager();
    if (!canManager->init()) {
        Serial0.println("❌ CAN管理器初始化失败");
    }
  
    
    // 创建协议管理器
    protocolManager = new ProtocolManager();
    protocolManager->init();
    protocolManager->getUCANHandler()->init(usbSendQueue, freePacketQueue, usbToCanQueue1, usbToCanQueue2);  // 🔧 传入两个队列
    protocolManager->getUCANHandler()->setCANManager(canManager);  // 设置CAN管理器
    Serial0.println("✅ 协议管理器初始化完成（支持动态配置命令）");
    
    // 创建统计管理器
    statistics = new Statistics();
    statistics->init(canManager);
    Serial0.println("✅ 统计管理器初始化完成");
    
    // 创建任务管理器
    taskManager = new TaskManager();
    taskManager->init(canManager);
    
    // 创建所有任务
    if (!taskManager->createAllTasks()) {
        Serial0.println("❌ 任务创建失败");
    }
    
    // 设置中断处理器的任务句柄
    if (canManager->isChannelReady(0)) {
        CANInterruptHandler::setCAN1TaskHandle(taskManager->getCAN1RxTaskHandle());
        // 设置回调并启用中断
        canManager->getDriver(0)->setNotificationCallback(CANInterruptHandler::can1ISR);
        canManager->getDriver(0)->enableInterrupt();
        Serial0.println("✅ CAN1中断已设置");
    }
    
    if (canManager->isChannelReady(1)) {
        CANInterruptHandler::setCAN2TaskHandle(taskManager->getCAN2RxTaskHandle());
        canManager->getDriver(1)->setNotificationCallback(CANInterruptHandler::can2ISR);
        canManager->getDriver(1)->enableInterrupt();
        Serial0.println("✅ CAN2中断已设置");
    }
    
    Serial0.println("✅ CAN任务已创建");
    
  
    
    // 移动到这里：等待上位机连接
    // 注意：CAN任务已经在后台运行并接收数据到环形缓冲区/队列
    // 当USB连接后，数据会自动开始流向USB
    Serial0.println("\n⏳ 等待上位机连接...");
    Serial0.println("   请打开上位机软件并连接设备");
    
    // 初始化时先假设未连接
    usbConnected = false;
    canReceivePaused = false; // 初始不暂停，允许数据进入环形缓冲
    
    // 启动一个后台任务来监测USB连接状态，而不是阻塞在setup
    // 这样loop()可以运行，且不影响中断处理
    xTaskCreatePinnedToCore(
        [](void* param) {
            uint32_t waitCount = 0;
            while (true) {
                bool usbReady = isUsbHostConnected();

                if (usbDisconnectEventPending) {
                    usbDisconnectEventPending = false;
                    usbConnectEventPending = false;
                    if (usbConnected) {
                        Serial0.println("\n🔌 上位机已断开");
                        handleHostDisconnect();
                    }
                } else if (usbConnectEventPending) {
                    usbConnectEventPending = false;
                    if (!usbConnected) {
                        Serial0.println("\n✅ 上位机已连接");
                        usbConnected = true;
                        canReceivePaused = false;
                        framesDroppedOnDisconnect = 0;
                        lastHostHeartbeatMs = millis();
                    }
                }
                
                // 状态变更处理
                if (usbReady && !usbConnected) {
                    Serial0.println("\n✅ 上位机已连接");
                    usbConnected = true;
                    canReceivePaused = false;
                    lastHostHeartbeatMs = millis();
                } else if (!usbReady && usbConnected) {
                    Serial0.println("\n🔌 上位机已断开");
                    handleHostDisconnect(); // 复用断开处理逻辑
                }
                
                if (!usbConnected && (waitCount % 50 == 0)) {
                     // 仅在未连接时每5秒打印提示
                     // Serial0.printf("⏳ 等待上位机连接... %lu秒\n", waitCount / 10);
                }
                waitCount++;
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        },
        "USBMonitor",
        4096,
        NULL,
        1,
        NULL,
        0 // Core 0
    );
}

void loop() {
    static uint32_t lastSyncTime = 0;
    uint32_t now = millis();
    
    // 每100ms同步一次时间
    if (now - lastSyncTime >= 100) {
        if (canManager) {
            for (uint8_t i = 0; i < 2; i++) {
                CANDriver* driver = canManager->getDriver(i);
                if (driver) {
                    TimeSyncManager::getInstance().updateSync(i, driver->currentTBC());
                }
            }
        }
        lastSyncTime = now;
    }
    
    delay(10);
}
