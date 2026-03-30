#ifndef ZCAN_CONFIG_H
#define ZCAN_CONFIG_H

/**
 * @file zcan_config.h
 * @brief ZCAN协议配置常量
 */

namespace ZCAN {

// ============================================================================
// 批量传输配置
// ============================================================================

/// 默认批量大小（帧数）
constexpr int DEFAULT_BATCH_SIZE = 50;

/// 最小批量大小
constexpr int MIN_BATCH_SIZE = 1;

/// 最大批量大小
constexpr int MAX_BATCH_SIZE = 255;

/// 固件侧批量上限（ESP32固件的BATCH_MAX_FRAMES）
/// 用于失步检测：如果收到的帧数量>此值，说明数据流失步
/// 与固件 UCAN STREAM_SET 对齐：maxFrames 通常 1..50
constexpr int FIRMWARE_MAX_BATCH_SIZE = 50;

/// 批量超时时间（毫秒）
constexpr int BATCH_TIMEOUT_MS = 5;

/// 时间戳偏移最大值（微秒）- 2字节uint16最大值
constexpr int MAX_TIME_OFFSET_US = 65535;

// ============================================================================
// 内存池配置
// ============================================================================

/// 内存池大小（预分配CANFrame数量）
constexpr int FRAME_POOL_SIZE = 10000;

/// 批量帧缓冲区大小
constexpr int BATCH_BUFFER_SIZE = 1000;

// ============================================================================
// 压缩配置
// ============================================================================

/// 最小压缩大小（字节）- 小于此值不压缩
constexpr int MIN_COMPRESS_SIZE = 64;

/// 最大压缩缓冲区大小（字节）
constexpr int MAX_COMPRESS_BUFFER = 65536;

/// 差分编码最大帧数
constexpr int MAX_DIFFERENTIAL_FRAMES = 100;

// ============================================================================
// 协议协商配置
// ============================================================================

/// 协议版本号
constexpr quint16 PROTOCOL_VERSION = 0x0100;  // v1.0

/// 协商超时时间（毫秒）
constexpr int NEGOTIATE_TIMEOUT_MS = 1000;

/// 协商重试次数
constexpr int NEGOTIATE_RETRY_COUNT = 3;

// ============================================================================
// 性能配置
// ============================================================================

/// 零拷贝解析启用标志
constexpr bool ENABLE_ZERO_COPY = true;

/// 零拷贝性能提升预期（相比逐字节解析）
constexpr int ZERO_COPY_SPEEDUP_PERCENT = 40;  // 预期提升40%

/// 多线程启用标志
/// 说明：当前单线程+零拷贝已能满足性能需求（26,000帧/秒@4Mbps）
/// 多线程会增加代码复杂度和调试难度，暂不启用
/// 如需更高性能，可在未来版本中实现三线程流水线架构
constexpr bool ENABLE_MULTITHREAD = false;

/// SIMD优化启用标志（需要AVX2支持）
/// 说明：SIMD优化需要特定CPU支持，收益相对较小（约20-30%）
/// 当前零拷贝优化已提供足够性能提升，SIMD优化可作为未来增强
constexpr bool ENABLE_SIMD = false;

// ============================================================================
// 调试配置
// ============================================================================

/// 调试日志启用标志
constexpr bool ENABLE_DEBUG_LOG = true;

/// 性能统计启用标志
constexpr bool ENABLE_STATISTICS = true;

/// 详细日志间隔（每N帧打印一次）
constexpr int DEBUG_LOG_INTERVAL = 100;

} // namespace ZCAN

#endif // ZCAN_CONFIG_H
