#include "ZCANStatistics.h"
#include <QDebug>
#include <cmath>

namespace ZCAN {

ZCANStatistics::ZCANStatistics(QObject *parent)
    : QObject(parent)
    , m_running(false)
    , m_maxBatchFrames(255)
    , m_framesReceived(0)
    , m_framesSent(0)
    , m_framesError(0)
    , m_framesDropped(0)
    , m_bytesReceived(0)
    , m_bytesSent(0)
    , m_batchCount(0)
    , m_totalBatchSize(0)
    , m_compressCount(0)
    , m_originalBytes(0)
    , m_compressedBytes(0)
    , m_totalLatencyUs(0)
    , m_minLatencyUs(999999.0)
    , m_maxLatencyUs(0)
    , m_latencySamples(0)
{
}

ZCANStatistics::~ZCANStatistics()
{
}

void ZCANStatistics::start()
{
    QMutexLocker locker(&m_mutex);
    m_timer.start();
    m_running = true;
}

void ZCANStatistics::stop()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
}

void ZCANStatistics::reset()
{
    QMutexLocker locker(&m_mutex);
    
    m_framesReceived = 0;
    m_framesSent = 0;
    m_framesError = 0;
    m_framesDropped = 0;
    m_bytesReceived = 0;
    m_bytesSent = 0;
    
    m_batchCount = 0;
    m_totalBatchSize = 0;
    
    m_compressCount = 0;
    m_originalBytes = 0;
    m_compressedBytes = 0;
    
    m_totalLatencyUs = 0;
    m_minLatencyUs = 999999.0;
    m_maxLatencyUs = 0;
    m_latencySamples = 0;
    
    // 如果统计器正在运行，重新启动计时器
    if (m_running) {
        m_timer.restart();
    }
}

void ZCANStatistics::recordFramesReceived(int frameCount, int bytes, double latencyUs)
{
    QMutexLocker locker(&m_mutex);
    
    m_framesReceived += frameCount;
    m_bytesReceived += bytes;
    
    // 记录延迟
    if (latencyUs > 0) {
        m_totalLatencyUs += latencyUs;
        m_latencySamples++;
        
        if (latencyUs < m_minLatencyUs) {
            m_minLatencyUs = latencyUs;
        }
        if (latencyUs > m_maxLatencyUs) {
            m_maxLatencyUs = latencyUs;
        }
    }
}

void ZCANStatistics::recordFramesSent(int frameCount, int bytes)
{
    QMutexLocker locker(&m_mutex);
    
    m_framesSent += frameCount;
    m_bytesSent += bytes;
}

void ZCANStatistics::recordBatch(int batchSize)
{
    QMutexLocker locker(&m_mutex);
    
    m_batchCount++;
    m_totalBatchSize += batchSize;
}

void ZCANStatistics::setMaxBatchFrames(int maxFrames)
{
    QMutexLocker locker(&m_mutex);
    m_maxBatchFrames = maxFrames;
}

void ZCANStatistics::recordCompression(int originalSize, int compressedSize)
{
    QMutexLocker locker(&m_mutex);
    
    m_compressCount++;
    m_originalBytes += originalSize;
    m_compressedBytes += compressedSize;
}

void ZCANStatistics::recordError()
{
    QMutexLocker locker(&m_mutex);
    m_framesError++;
}

void ZCANStatistics::recordDropped()
{
    QMutexLocker locker(&m_mutex);
    m_framesDropped++;
}

PerformanceMetrics ZCANStatistics::getMetrics() const
{
    QMutexLocker locker(&m_mutex);
    
    PerformanceMetrics metrics = {};
    
    if (!m_running) {
        return metrics;
    }
    
    qint64 elapsedMs = m_timer.elapsed();
    if (elapsedMs == 0) {
        return metrics;
    }
    
    double elapsedSec = elapsedMs / 1000.0;
    
    // 吞吐量指标
    metrics.framesReceived = m_framesReceived;
    metrics.framesSent = m_framesSent;
    metrics.throughputFps = m_framesReceived / elapsedSec;
    
    // 计算Mbps（假设平均每帧20字节）
    double avgBytesPerFrame = m_framesReceived > 0 ? 
        static_cast<double>(m_bytesReceived) / m_framesReceived : 20.0;
    metrics.throughputMbps = (metrics.throughputFps * avgBytesPerFrame * 8) / 1000000.0;
    
    // 延迟指标
    if (m_latencySamples > 0) {
        metrics.avgLatencyUs = m_totalLatencyUs / m_latencySamples;
        metrics.minLatencyUs = m_minLatencyUs;
        metrics.maxLatencyUs = m_maxLatencyUs;
    }
    
    // 批量传输指标
    metrics.batchCount = m_batchCount;
    if (m_batchCount > 0) {
        metrics.avgBatchSize = static_cast<double>(m_totalBatchSize) / m_batchCount;
        // 批量效率 = (实际批量大小 / 配置的最大批量大小) × 100%
        const int denom = (m_maxBatchFrames > 0) ? m_maxBatchFrames : 255;
        metrics.batchEfficiency = (metrics.avgBatchSize / static_cast<double>(denom)) * 100.0;
    }
    
    // 压缩指标
    metrics.compressCount = m_compressCount;
    metrics.originalBytes = m_originalBytes;
    metrics.compressedBytes = m_compressedBytes;
    if (m_originalBytes > 0) {
        // 压缩率 = (1 - 压缩后大小/原始大小) × 100%
        metrics.compressionRatio = (1.0 - static_cast<double>(m_compressedBytes) / m_originalBytes) * 100.0;
    }
    
    // 错误指标
    metrics.framesError = m_framesError;
    metrics.framesDropped = m_framesDropped;
    // 🔧 修复：确保即使只有接收帧，也能计算出丢包率
    // 丢包率 = 丢包数 / (接收数 + 丢包数 + 错误数)
    quint32 totalFrames = m_framesReceived + m_framesError + m_framesDropped;
    if (totalFrames > 0) {
        metrics.errorRate = (static_cast<double>(m_framesError + m_framesDropped) / totalFrames) * 100.0;
    } else {
        metrics.errorRate = 0.0;
    }
    
    // 运行时间
    metrics.uptimeMs = elapsedMs;
    
    return metrics;
}

double ZCANStatistics::getThroughputFps() const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_running) {
        return 0;
    }
    
    qint64 elapsedMs = m_timer.elapsed();
    if (elapsedMs == 0) {
        return 0;
    }
    
    double elapsedSec = elapsedMs / 1000.0;
    return m_framesReceived / elapsedSec;
}

double ZCANStatistics::getAvgLatencyUs() const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_latencySamples == 0) {
        return 0;
    }
    
    return m_totalLatencyUs / m_latencySamples;
}

double ZCANStatistics::getCompressionRatio() const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_originalBytes == 0) {
        return 0;
    }
    
    return (1.0 - static_cast<double>(m_compressedBytes) / m_originalBytes) * 100.0;
}

double ZCANStatistics::getErrorRate() const
{
    QMutexLocker locker(&m_mutex);
    
    quint32 totalFrames = m_framesReceived + m_framesError + m_framesDropped;
    if (totalFrames == 0) {
        return 0;
    }
    
    return (static_cast<double>(m_framesError + m_framesDropped) / totalFrames) * 100.0;
}

void ZCANStatistics::printStatistics() const
{
    PerformanceMetrics metrics = getMetrics();
    
    qDebug() << "========== ZCAN性能统计 ==========";
    qDebug() << "运行时间:" << metrics.uptimeMs / 1000.0 << "秒";
    qDebug() << "";
    qDebug() << "【吞吐量】";
    qDebug() << "  接收帧数:" << metrics.framesReceived;
    qDebug() << "  发送帧数:" << metrics.framesSent;
    qDebug() << "  吞吐量:" << QString::number(metrics.throughputFps, 'f', 2) << "帧/秒";
    qDebug() << "  吞吐量:" << QString::number(metrics.throughputMbps, 'f', 2) << "Mbps";
    qDebug() << "";
    qDebug() << "【延迟】";
    qDebug() << "  平均延迟:" << QString::number(metrics.avgLatencyUs, 'f', 2) << "μs";
    qDebug() << "  最小延迟:" << QString::number(metrics.minLatencyUs, 'f', 2) << "μs";
    qDebug() << "  最大延迟:" << QString::number(metrics.maxLatencyUs, 'f', 2) << "μs";
    qDebug() << "";
    qDebug() << "【批量传输】";
    qDebug() << "  批量次数:" << metrics.batchCount;
    qDebug() << "  平均批量:" << QString::number(metrics.avgBatchSize, 'f', 2) << "帧";
    qDebug() << "  批量效率:" << QString::number(metrics.batchEfficiency, 'f', 2) << "%";
    qDebug() << "";
    qDebug() << "【压缩】";
    qDebug() << "  压缩次数:" << metrics.compressCount;
    qDebug() << "  原始大小:" << metrics.originalBytes << "字节";
    qDebug() << "  压缩大小:" << metrics.compressedBytes << "字节";
    qDebug() << "  压缩率:" << QString::number(metrics.compressionRatio, 'f', 2) << "%";
    qDebug() << "";
    qDebug() << "【错误】";
    qDebug() << "  错误帧数:" << metrics.framesError;
    qDebug() << "  丢帧数:" << metrics.framesDropped;
    qDebug() << "  错误率:" << QString::number(metrics.errorRate, 'f', 4) << "%";
    qDebug() << "==================================";
}

bool ZCANStatistics::isRunning() const
{
    QMutexLocker locker(&m_mutex);
    return m_running;
}

} // namespace ZCAN
