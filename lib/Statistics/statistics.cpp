/*
 * ESP32-S3 UCAN Gateway - 性能统计实现
 */

#include "statistics.h"
#include "../Config/performance_config.h"

// 外部全局变量（在main.cpp中定义）
extern volatile uint32_t totalFramesReceived;
extern volatile uint32_t totalBatchesSent;
// extern bool batchModeEnabled; // 已废弃
extern QueueHandle_t freePacketQueue;
extern QueueHandle_t usbSendQueue;
extern volatile uint32_t poolEmptyCount;
extern volatile uint32_t queueFullCount;
extern volatile uint32_t buildFailedCount;

Statistics::Statistics()
    : m_canManager(nullptr)
    , m_totalFramesReceived(0)
    , m_totalBatchesSent(0)
    , m_lastFrameCount(0)
    , m_lastCan1RxCount(0)
    , m_lastCan2RxCount(0)
    , m_lastUpdateTime(0)
{
    memset(&m_metrics, 0, sizeof(m_metrics));
}

bool Statistics::init(CANManager* canManager) {
    m_canManager = canManager;
    m_lastUpdateTime = millis();
    return true;
}

void Statistics::update() {
    uint64_t now = millis();
    uint64_t elapsed = now - m_lastUpdateTime;
    
    if (elapsed < 1000) {
        return;  // 每秒更新一次
    }
    
    // 计算帧率
    uint32_t currentCount = totalFramesReceived;
    m_metrics.totalFrameRate = (currentCount - m_lastFrameCount) * 1000 / elapsed;
    m_lastFrameCount = currentCount;
    m_lastUpdateTime = now;
    
    // 更新总计数
    m_totalFramesReceived = totalFramesReceived;
    m_totalBatchesSent = totalBatchesSent;
    
    // 获取CAN驱动统计
    if (m_canManager) {
        CANDriverStats stats;
        
        // CAN1统计
        if (m_canManager->isChannelReady(0)) {
            m_canManager->getStats(0, stats);
            m_metrics.can1FrameRate = (stats.rxCount - m_lastCan1RxCount) * 1000 / elapsed;
            m_lastCan1RxCount = stats.rxCount;
        }
        
        // CAN2统计
        if (m_canManager->isChannelReady(1)) {
            m_canManager->getStats(1, stats);
            m_metrics.can2FrameRate = (stats.rxCount - m_lastCan2RxCount) * 1000 / elapsed;
            m_lastCan2RxCount = stats.rxCount;
        }
    }
    
    // 计算内存占用
    m_metrics.memoryUsage = ESP.getFreeHeap();
    
    // 计算内存池使用率
    if (freePacketQueue) {
        UBaseType_t freeCount = uxQueueMessagesWaiting(freePacketQueue);
        UBaseType_t totalCount = QUEUE_USB_SEND_SIZE;
        uint32_t usedCount = totalCount - freeCount;
        m_metrics.poolUsage = (usedCount * 100) / totalCount;
    }
    
    // 计算队列深度
    if (usbSendQueue) {
        m_metrics.queueDepth = uxQueueMessagesWaiting(usbSendQueue);
    }
    
    // 更新错误统计
    m_metrics.poolEmptyCount = poolEmptyCount;
    m_metrics.queueFullCount = queueFullCount;
    m_metrics.buildFailedCount = buildFailedCount;
    
    // 估算CPU占用率（基于帧率和中断频率）
    // 假设每帧处理需要约20μs，每次中断需要约10μs
    uint32_t frameProcessingUs = m_metrics.totalFrameRate * 20;  // 微秒/秒
    uint32_t interruptUs = m_metrics.totalFrameRate * 10;        // 微秒/秒
    uint32_t totalUs = frameProcessingUs + interruptUs;
    m_metrics.cpuUsage = (float)totalUs / 10000.0f;  // 转换为百分比
    
    // 估算平均延迟（基于批量大小和超时）
    if (m_totalBatchesSent > 0 && m_totalFramesReceived > 0) {
        uint32_t avgBatchSize = m_totalFramesReceived / m_totalBatchesSent;
        // 延迟 = 批量超时 / 2（平均情况）
        // 根据实际帧率动态计算延迟
        uint32_t timeoutMs = 2;  // 默认高速
        if (m_metrics.totalFrameRate < 1000) {
            timeoutMs = 20;  // 低速: 20ms
        } else if (m_metrics.totalFrameRate < 5000) {
            timeoutMs = 5;   // 中速: 5ms
        }
        m_metrics.avgLatency = (timeoutMs * 1000) / 2;  // 微秒
    }
}

void Statistics::print() {
    Serial0.println("╔════════════════════════════════════════════════════════════╗");
    Serial0.printf("║ 📊 性能统计 - 帧率: %lu 帧/秒 | CPU: %.1f%% | 延迟: %lu μs ║\n",
                 m_metrics.totalFrameRate,
                 m_metrics.cpuUsage,
                 m_metrics.avgLatency);
    Serial0.println("╠════════════════════════════════════════════════════════════╣");
    Serial0.printf("║ CAN1: %5lu 帧/秒 | CAN2: %5lu 帧/秒                      ║\n",
                 m_metrics.can1FrameRate,
                 m_metrics.can2FrameRate);
    Serial0.printf("║ 总接收: %10lu 帧 | 批次: %10lu                  ║\n",
                 m_totalFramesReceived,
                 m_totalBatchesSent);
    const bool isBatch = true;
    
    Serial0.printf("║ 内存: %lu KB 可用 | 模式: %-20s          ║\n",
                 m_metrics.memoryUsage / 1024,
                 isBatch ? "UCAN批量" : "标准");
    
    // 资源监控（新增）
    Serial0.println("╠════════════════════════════════════════════════════════════╣");
    Serial0.printf("║ 📦 资源监控                                                ║\n");
    Serial0.printf("║ 内存池使用率: %lu%% | 队列深度: %lu                       ║\n",
                 m_metrics.poolUsage,
                 m_metrics.queueDepth);
    
    // 错误统计（仅在有错误时显示）
    if (m_metrics.poolEmptyCount > 0 || m_metrics.queueFullCount > 0 || m_metrics.buildFailedCount > 0) {
        Serial0.println("╠════════════════════════════════════════════════════════════╣");
        Serial0.printf("║ ⚠️  错误统计                                               ║\n");
        if (m_metrics.poolEmptyCount > 0) {
            Serial0.printf("║ 内存池耗尽: %lu 次                                        ║\n", m_metrics.poolEmptyCount);
        }
        if (m_metrics.queueFullCount > 0) {
            Serial0.printf("║ 队列满: %lu 次                                            ║\n", m_metrics.queueFullCount);
        }
        if (m_metrics.buildFailedCount > 0) {
            Serial0.printf("║ 构建失败: %lu 次                                          ║\n", m_metrics.buildFailedCount);
        }
    }
    
    // 输出CAN驱动详细统计
    if (m_canManager) {
        CANDriverStats stats;
        
        Serial0.println("╠════════════════════════════════════════════════════════════╣");
        if (m_canManager->isChannelReady(0)) {
            m_canManager->getStats(0, stats);
            Serial0.printf("║ CAN1详情: RX=%lu, TX=%lu, INT=%lu, ERR=%lu           ║\n",
                         stats.rxCount, stats.txCount, stats.interruptCount, stats.errorCount);
        }
        
        if (m_canManager->isChannelReady(1)) {
            m_canManager->getStats(1, stats);
            Serial0.printf("║ CAN2详情: RX=%lu, TX=%lu, INT=%lu, ERR=%lu           ║\n",
                         stats.rxCount, stats.txCount, stats.interruptCount, stats.errorCount);
        }
    }
    
    Serial0.println("╚════════════════════════════════════════════════════════════╝");
}

void Statistics::recordRxFrame(uint8_t channel) {
    // 由外部全局变量管理
}

void Statistics::recordTxFrame(uint8_t channel) {
    // 由外部全局变量管理
}

void Statistics::recordBatch() {
    // 由外部全局变量管理
}
