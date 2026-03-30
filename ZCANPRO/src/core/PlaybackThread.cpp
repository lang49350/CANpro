#include "PlaybackThread.h"
#include "ConnectionManager.h"
#include <QElapsedTimer>
#include <QThread>
#include <QDebug>
#include <thread>
#include <chrono>

/**
 * @brief 构造函数
 */
PlaybackThread::PlaybackThread(QObject *parent)
    : QThread(parent)
    , m_provider(nullptr)
    , m_sourceChannelFilter(-1)
    , m_speed(1.0)
    , m_useOriginalTiming(true)
    , m_stepIntervalMs(10)
    , m_state(0) // Stopped
    , m_abort(false)
{
    // 初始化通道映射
    m_channelMap.clear();
}

/**
 * @brief 析构函数
 */
PlaybackThread::~PlaybackThread()
{
    stopPlayback();
    wait();
    if (m_provider) {
        delete m_provider;
        m_provider = nullptr;
    }
}

/**
 * @brief 设置数据提供者
 */
void PlaybackThread::setProvider(IFrameProvider *provider)
{
    QMutexLocker locker(&m_mutex);
    if (m_provider) {
        delete m_provider;
    }
    m_provider = provider;
    
    // 如果 provider 是 MemoryFrameProvider，则应用通道过滤
    MemoryFrameProvider *memProvider = dynamic_cast<MemoryFrameProvider*>(m_provider);
    if (memProvider) {
        memProvider->setChannelFilter(m_sourceChannelFilter);
    }
}

/**
 * @brief 设置回放设备
 */
void PlaybackThread::setDevice(const QString &device)
{
    QMutexLocker locker(&m_mutex);
    m_device = device;
}

/**
 * @brief 设置通道映射
 */
void PlaybackThread::setChannelMapping(int source, int target)
{
    QMutexLocker locker(&m_mutex);
    m_channelMap.insert(source, target);
}

/**
 * @brief 清除通道映射
 */
void PlaybackThread::clearChannelMapping()
{
    QMutexLocker locker(&m_mutex);
    m_channelMap.clear();
}

/**
 * @brief 设置源通道过滤
 */
void PlaybackThread::setSourceChannelFilter(int channel)
{
    QMutexLocker locker(&m_mutex);
    m_sourceChannelFilter = channel;
    
    // 如果当前有 provider，则更新其过滤
    MemoryFrameProvider *memProvider = dynamic_cast<MemoryFrameProvider*>(m_provider);
    if (memProvider) {
        memProvider->setChannelFilter(m_sourceChannelFilter);
    }
}

/**
 * @brief 设置回放速度
 */
void PlaybackThread::setSpeed(double speed)
{
    QMutexLocker locker(&m_mutex);
    if (speed <= 0.0) speed = 1.0;
    if (speed > 5.0) speed = 5.0;
    m_speed = speed;
}

void PlaybackThread::setUseOriginalTiming(bool useOriginalTiming)
{
    QMutexLocker locker(&m_mutex);
    m_useOriginalTiming = useOriginalTiming;
}

void PlaybackThread::setStepIntervalMs(int stepIntervalMs)
{
    QMutexLocker locker(&m_mutex);
    if (stepIntervalMs < 1) stepIntervalMs = 1;
    if (stepIntervalMs > 1000) stepIntervalMs = 1000;
    m_stepIntervalMs = stepIntervalMs;
}

/**
 * @brief 启动回放
 */
void PlaybackThread::startPlayback()
{
    if (isRunning()) return;

    m_abort = false;
    m_state = 1; // Running
    start(QThread::HighPriority); // 设置高优先级以保证实时性
}

/**
 * @brief 暂停回放
 */
void PlaybackThread::pausePlayback()
{
    QMutexLocker locker(&m_mutex);
    if (m_state == 1) {
        m_state = 2; // Paused
    }
}

/**
 * @brief 恢复回放
 */
void PlaybackThread::resumePlayback()
{
    QMutexLocker locker(&m_mutex);
    if (m_state == 2) {
        m_state = 1; // Running
        m_cond.wakeOne();
    }
}

/**
 * @brief 停止回放
 */
void PlaybackThread::stopPlayback()
{
    m_abort = true;
    m_state = 0; // Stopped
    m_cond.wakeOne();
}

/**
 * @brief 状态查询
 */
bool PlaybackThread::isPlaying() const
{
    return m_state == 1;
}

bool PlaybackThread::isPaused() const
{
    return m_state == 2;
}

/**
 * @brief 线程执行函数
 */
void PlaybackThread::run()
{
    if (!m_provider) {
        emit errorOccurred("没有可播放的数据");
        return;
    }

    m_provider->reset();
    
    // 初始化计时器
    QElapsedTimer timer;
    timer.start();
    
    // 暂停补偿时间
    qint64 pauseOffsetUs = 0;
    
    // 读取第一帧的时间戳作为基准
    CANFrame frame;
    if (!m_provider->hasNext()) {
        emit playbackFinished();
        return;
    }

    if (!m_provider->next(frame)) {
        emit playbackFinished();
        return;
    }
    
    quint64 startTimestamp = frame.timestamp;
    quint64 framesSent = 0;
    
    // 发送第一帧
    // 拷贝局部变量以减少锁争用
    QString device;
    QMap<int, int> channelMap;
    double speed;
    bool useOriginalTiming;
    int stepIntervalMs;
    {
        QMutexLocker locker(&m_mutex);
        device = m_device;
        channelMap = m_channelMap;
        speed = m_speed;
        useOriginalTiming = m_useOriginalTiming;
        stepIntervalMs = m_stepIntervalMs;
    }

    int channel = frame.channel;
    if (channelMap.contains(channel)) {
        frame.channel = channelMap.value(channel);
    }
    // 首帧优先同步立即发送，保证回放起点时间一致
    if (!ConnectionManager::instance()->sendFrameSyncImmediate(device, frame)) {
        // 若立即发送失败，回退到发送队列，避免直接丢首帧
        ConnectionManager::instance()->enqueueSendFrame(device, frame);
    }
    framesSent++;

    // 主循环
    while (!m_abort && m_provider->hasNext()) {
        // 处理暂停
        if (m_state == 2) { // Paused
            QMutexLocker locker(&m_mutex);
            qint64 pauseStart = timer.nsecsElapsed();
            while (m_state == 2 && !m_abort) {
                m_cond.wait(&m_mutex);
            }
            if (m_abort) break;
            
            // 恢复后，加上暂停的时长
            qint64 pauseDuration = timer.nsecsElapsed() - pauseStart;
            pauseOffsetUs += (pauseDuration / 1000);
            
            // 重新读取配置（暂停期间可能修改了配置）
            device = m_device;
            channelMap = m_channelMap;
            speed = m_speed;
            useOriginalTiming = m_useOriginalTiming;
            stepIntervalMs = m_stepIntervalMs;
        }
        
        if (m_abort) break;

        // 读取下一帧
        if (!m_provider->next(frame)) break;

        // 计算目标发送时间（相对于开始）
        // targetTimeUs = (frame.timestamp - startTimestamp) / speed + pauseOffsetUs
        qint64 targetTimeUs = 0;
        if (useOriginalTiming) {
            quint64 relativeTimeUs = frame.timestamp - startTimestamp;
            double scaledTimeUs = static_cast<double>(relativeTimeUs) / speed;
            targetTimeUs = static_cast<qint64>(scaledTimeUs) + pauseOffsetUs;
        } else {
            double perFrameIntervalUs = (static_cast<double>(stepIntervalMs) * 1000.0) / speed;
            if (perFrameIntervalUs < 50.0) perFrameIntervalUs = 50.0;
            targetTimeUs = static_cast<qint64>(framesSent * perFrameIntervalUs) + pauseOffsetUs;
        }
        
        // 高精度等待
        qint64 currentUs = timer.nsecsElapsed() / 1000;
        qint64 waitUs = targetTimeUs - currentUs;
        
        if (waitUs > 0) {
            highPrecisionSleep(waitUs);
        }

        // Disconnected during sleep: pause should take effect immediately,
        // preventing extra enqueue/send.
        if (m_abort) break;
        if (m_state == 2) { // Paused
            continue;
        }
        
        // 应用通道映射
        channel = frame.channel;
        if (channelMap.contains(channel)) {
            frame.channel = channelMap.value(channel);
        }
        
        // 发送帧
        // 使用无锁队列入队，避免阻塞
        ConnectionManager::instance()->enqueueSendFrame(device, frame);
        
        framesSent++;
        
        // 更新进度
        if (framesSent % 100 == 0) {
            emit progressUpdated(m_provider->currentIndex(), m_provider->totalFrames());
        }
    }
    
    emit progressUpdated(m_provider->currentIndex(), m_provider->totalFrames());
    emit playbackFinished();
}

/**
 * @brief 高精度睡眠
 * @param microseconds 微秒
 */
void PlaybackThread::highPrecisionSleep(qint64 microseconds)
{
    if (microseconds <= 0) return;
    
    // 如果等待时间较长 (>2ms)，使用系统睡眠让出 CPU
    if (microseconds > 2000) {
        // 留出 1ms 的余量给忙等待
        QThread::msleep(static_cast<unsigned long>((microseconds - 1000) / 1000));
    }
    
    // 剩下的时间使用忙等待（Spin Lock）以保证微秒级精度
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::microseconds(microseconds);
    
    while (std::chrono::high_resolution_clock::now() < end) {
        // _mm_pause() 在 MSVC 下需要 <intrin.h>，且可能不被所有编译器支持
        // std::this_thread::yield() 可能会有较大延迟
        // 空循环即可，或者可以使用 std::atomic_thread_fence(std::memory_order_relaxed) 防止编译器优化
        std::atomic_thread_fence(std::memory_order_relaxed);
    }
}
