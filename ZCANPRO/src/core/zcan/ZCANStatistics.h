#ifndef ZCAN_STATISTICS_H
#define ZCAN_STATISTICS_H

#include <QObject>
#include <QElapsedTimer>
#include <QMutex>
#include "zcan_types.h"

/**
 * @file ZCANStatistics.h
 * @brief ZCAN性能统计模块
 * 
 * 提供实时性能监控：
 * - 吞吐量统计（帧/秒）
 * - 延迟统计（微秒/帧）
 * - 压缩率统计
 * - CPU占用监控
 * - 内存占用监控
 */

namespace ZCAN {

/**
 * @brief 性能统计数据结构
 */
struct PerformanceMetrics {
    // 吞吐量指标
    quint32 framesReceived;      ///< 接收帧总数
    quint32 framesSent;          ///< 发送帧总数
    double throughputFps;        ///< 吞吐量（帧/秒）
    double throughputMbps;       ///< 吞吐量（Mbps）
    
    // 延迟指标
    double avgLatencyUs;         ///< 平均延迟（微秒）
    double minLatencyUs;         ///< 最小延迟（微秒）
    double maxLatencyUs;         ///< 最大延迟（微秒）
    
    // 批量传输指标
    quint32 batchCount;          ///< 批量传输次数
    double avgBatchSize;         ///< 平均批量大小
    double batchEfficiency;      ///< 批量效率（%）
    
    // 压缩指标
    quint32 compressCount;       ///< 压缩次数
    quint64 originalBytes;       ///< 原始字节数
    quint64 compressedBytes;     ///< 压缩后字节数
    double compressionRatio;     ///< 压缩率（%）
    
    // 错误指标
    quint32 framesError;         ///< 错误帧数
    quint32 framesDropped;       ///< 丢帧数
    double errorRate;            ///< 错误率（%）
    
    // 资源占用
    double cpuUsage;             ///< CPU占用率（%）
    quint64 memoryUsage;         ///< 内存占用（字节）
    
    // 运行时间
    qint64 uptimeMs;             ///< 运行时间（毫秒）
};

/**
 * @brief ZCAN性能统计器
 */
class ZCANStatistics : public QObject
{
    Q_OBJECT
    
public:
    explicit ZCANStatistics(QObject *parent = nullptr);
    ~ZCANStatistics();
    
    /**
     * @brief 启动统计
     */
    void start();
    
    /**
     * @brief 停止统计
     */
    void stop();
    
    /**
     * @brief 重置统计
     */
    void reset();
    
    /**
     * @brief 记录接收帧
     * @param frameCount 帧数量
     * @param bytes 字节数
     * @param latencyUs 延迟（微秒）
     */
    void recordFramesReceived(int frameCount, int bytes, double latencyUs = 0);
    
    /**
     * @brief 记录发送帧
     * @param frameCount 帧数量
     * @param bytes 字节数
     */
    void recordFramesSent(int frameCount, int bytes);
    
    /**
     * @brief 记录批量传输
     * @param batchSize 批量大小
     */
    void recordBatch(int batchSize);

    // 设置批量模式下的最大帧数（用于计算批量效率口径）
    void setMaxBatchFrames(int maxFrames);
    
    /**
     * @brief 记录压缩
     * @param originalSize 原始大小
     * @param compressedSize 压缩后大小
     */
    void recordCompression(int originalSize, int compressedSize);
    
    /**
     * @brief 记录错误
     */
    void recordError();
    
    /**
     * @brief 记录丢帧
     */
    void recordDropped();
    
    /**
     * @brief 获取性能指标
     * @return 性能指标结构
     */
    PerformanceMetrics getMetrics() const;
    
    /**
     * @brief 获取吞吐量（帧/秒）
     * @return 吞吐量
     */
    double getThroughputFps() const;
    
    /**
     * @brief 获取平均延迟（微秒）
     * @return 平均延迟
     */
    double getAvgLatencyUs() const;
    
    /**
     * @brief 获取压缩率（%）
     * @return 压缩率
     */
    double getCompressionRatio() const;
    
    /**
     * @brief 获取错误率（%）
     * @return 错误率
     */
    double getErrorRate() const;
    
    /**
     * @brief 打印统计信息
     */
    void printStatistics() const;
    
    /**
     * @brief 检查统计器是否正在运行
     * @return true=运行中, false=已停止
     */
    bool isRunning() const;
    
signals:
    /**
     * @brief 统计更新信号
     * @param metrics 性能指标
     */
    void statisticsUpdated(const PerformanceMetrics& metrics);
    
private:
    mutable QMutex m_mutex;
    QElapsedTimer m_timer;
    bool m_running;

    int m_maxBatchFrames;
    
    // 计数器
    quint32 m_framesReceived;
    quint32 m_framesSent;
    quint32 m_framesError;
    quint32 m_framesDropped;
    quint64 m_bytesReceived;
    quint64 m_bytesSent;
    
    // 批量统计
    quint32 m_batchCount;
    quint64 m_totalBatchSize;
    
    // 压缩统计
    quint32 m_compressCount;
    quint64 m_originalBytes;
    quint64 m_compressedBytes;
    
    // 延迟统计
    double m_totalLatencyUs;
    double m_minLatencyUs;
    double m_maxLatencyUs;
    quint32 m_latencySamples;
    
    void updateMetrics();
};

} // namespace ZCAN

#endif // ZCAN_STATISTICS_H
