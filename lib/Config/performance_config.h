/*
 * ESP32-S3 UCAN Gateway - 性能配置
 * 极致性能优化参数
 */

#ifndef PERFORMANCE_CONFIG_H
#define PERFORMANCE_CONFIG_H

// ============================================================================
// 性能模式
// ============================================================================
#define PERFORMANCE_MODE_EXTREME    true

// ============================================================================
// 优化开关
// ============================================================================
#define USE_INTERRUPT_DRIVEN        true    // 中断驱动
#define USE_BATCH_READ              true    // 批量读取
#define USE_ZERO_COPY               true    // 零拷贝
#define USE_FREERTOS_QUEUE          true    // FreeRTOS队列
#define USE_USB_DOUBLE_BUFFER       true    // USB双缓冲
#define USE_DYNAMIC_BATCH           true    // 动态批量策略

// ============================================================================
// FIFO中断配置（关键优化）
// ============================================================================
#define FIFO_INTERRUPT_THRESHOLD    10      // FIFO水位触发（10帧）
#define FIFO_TIMEOUT_US             100     // 超时触发（100微秒）

// ============================================================================
// 批量配置
// ============================================================================
#define BATCH_MAX_FRAMES            30      // 最大批量帧数 (适应上位机限制)
#define BATCH_TIMEOUT_MS            2       // 默认超时时间 2ms (高速场景)
#define BATCH_MAX_TIME_OFFSET       200000  // 🔧 修复：最大时间偏移（微秒）- 从65ms增加到200ms，减少溢出频率

// 🔧 修复：动态批量策略（保持快速响应，避免积压）
#define BATCH_HIGH_SPEED_FRAMES     30      // 高速: 30帧
#define BATCH_HIGH_SPEED_TIMEOUT    2       // 高速: 2ms (>5000帧/秒)
#define BATCH_HIGH_SPEED_THRESHOLD  5000    // 高速阈值: >5000帧/秒

#define BATCH_MID_SPEED_FRAMES      20      // 中速: 20帧
#define BATCH_MID_SPEED_TIMEOUT     5       // 🔧 保持5ms，避免积压（之前30ms导致断开）
#define BATCH_MID_SPEED_THRESHOLD   1000    // 中速阈值: >1000帧/秒

#define BATCH_LOW_SPEED_FRAMES      15      // 低速: 15帧
#define BATCH_LOW_SPEED_TIMEOUT     50      // 低速: 50ms (<1000帧/秒)

// ============================================================================
// CAN配置
// ============================================================================
#define CAN_AUTO_DETECT             true    // 自动检测CAN/CAN FD
#define CAN_FIFO_SIZE               31      // 最大FIFO (MCP2518FD硬件限制31)
#define CAN_DEFAULT_BITRATE         500000  // 默认波特率: 500kbps
#define CAN_FD_DATA_BITRATE         2000000 // CAN FD数据速率: 2Mbps

// ============================================================================
// 队列配置（优化：平衡内存池和PSRAM ring）
// ============================================================================
#define QUEUE_CAN_TO_USB_SIZE       8000   // CAN→USB队列深度
#define QUEUE_USB_TO_CAN_SIZE       7000    // USB→CAN队列深度（优化：2000→5000，避免发送队列满）
#define QUEUE_USB_SEND_SIZE         3000    // 🚀 USB发送队列深度（PSRAM）- 调整为3000，为系统和PSRAM ring预留安全空间

// 🚀 PSRAM环形缓冲区配置（不丢包策略）
#define USE_PSRAM_RING_BUFFER       1       // 启用PSRAM环形缓冲区
#define PSRAM_RING_SIZE             (1 * 1024 * 1024)  // 🔧 修复：从3MB降到1MB，避免满载阻塞
// ESP32-S3-N16R8有8MB PSRAM：
// - PSRAM ring：1MB（可缓冲约5秒）
// - 内存池：4000 packets × 1026字节 ≈ 4MB
// - 其他开销：约1MB
// - 剩余：约2MB（安全余量）
// 总计：约8MB

// ============================================================================
// 任务优先级配置（优化：提高CAN任务优先级）
// ============================================================================
// Core 0 (协议核心)
#define TASK_PRIORITY_USB_RX        7       // USB接收（提高优先级处理高速发送数据）
#define TASK_PRIORITY_USB_TX        4       // USB发送
#define TASK_PRIORITY_PROTOCOL      3       // 协议处理
#define TASK_PRIORITY_STATISTICS    1       // 统计

// Core 1 (CAN核心) - 优化：提高CAN任务优先级避免丢包
#define TASK_PRIORITY_CAN_ISR       10      // CAN中断处理（最高）
#define TASK_PRIORITY_CAN_BATCH     18      // 🚀 CAN批量处理（优化：15→18，最高优先级确保零丢包）
#define TASK_PRIORITY_CAN_TX        8       // CAN发送（优化：6→8，提高发送优先级）

// ============================================================================
// 任务栈大小配置
// ============================================================================
#define TASK_STACK_SIZE_USB_RX      4096
#define TASK_STACK_SIZE_USB_TX      4096
#define TASK_STACK_SIZE_PROTOCOL    4096
#define TASK_STACK_SIZE_STATISTICS  4096
#define TASK_STACK_SIZE_CAN_RX      16384  // 🔧 修复：8KB → 16KB（避免栈溢出）
#define TASK_STACK_SIZE_CAN_TX      4096

// ============================================================================
// 性能目标
// ============================================================================
#define TARGET_THROUGHPUT_CAN20     7000    // CAN 2.0目标吞吐量（帧/秒）
#define TARGET_THROUGHPUT_CANFD     12000   // CAN FD目标吞吐量（帧/秒）
#define TARGET_LATENCY_US           500     // 目标延迟（微秒）
#define TARGET_RELIABILITY          99.99   // 目标可靠性（%）

#endif // PERFORMANCE_CONFIG_H
