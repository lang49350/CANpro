#include "FramePlaybackCore.h"
#include "FrameFileParser.h"
#include "ConnectionManager.h"
#include "FrameProvider.h"
#include <QDebug>
#include <algorithm>

/**
 * @brief 构造函数
 */
FramePlaybackCore::FramePlaybackCore(QObject *parent)
    : QObject(parent)
    , m_thread(new PlaybackThread(this))
    , m_currentDevice("")
{
    // 连接线程信号
    connect(m_thread, &PlaybackThread::progressUpdated, this, &FramePlaybackCore::onThreadProgress);
    connect(m_thread, &PlaybackThread::playbackFinished, this, &FramePlaybackCore::onThreadFinished);
    connect(m_thread, &PlaybackThread::errorOccurred, this, &FramePlaybackCore::onThreadError);

    qDebug() << "✅ FramePlaybackCore 创建成功（独立线程版）";
}

/**
 * @brief 析构函数
 */
FramePlaybackCore::~FramePlaybackCore()
{
    if (m_thread) {
        m_thread->stopPlayback();
        m_thread->wait();
    }
    qDebug() << "🗑 FramePlaybackCore 销毁";
}

/**
 * @brief 设置设备
 */
void FramePlaybackCore::setDevice(const QString &device)
{
    m_currentDevice = device;
    m_thread->setDevice(device);
    qDebug() << "📡 FramePlaybackCore 设置设备:" << device;
}

/**
 * @brief 设置默认通道（兼容性接口）
 */
void FramePlaybackCore::setChannel(int channel)
{
    // 将所有常见通道映射到目标通道
    clearChannelMapping();
    for (int i = 0; i < 16; ++i) {
        setChannelMapping(i, channel);
    }
    qDebug() << "⚠️ 使用兼容接口 setChannel，已将通道 0-15 映射到:" << channel;
}

/**
 * @brief 设置通道映射
 */
void FramePlaybackCore::setChannelMapping(int source, int target)
{
    m_channelMap.insert(source, target);
    m_thread->setChannelMapping(source, target);
    qDebug() << "🔀 设置通道映射: " << source << " -> " << target;
}

/**
 * @brief 清除通道映射
 */
void FramePlaybackCore::clearChannelMapping()
{
    m_channelMap.clear();
    m_thread->clearChannelMapping();
    qDebug() << "🧹 清除通道映射";
}

/**
 * @brief 设置源通道过滤
 */
void FramePlaybackCore::setSourceChannelFilter(int channel)
{
    m_thread->setSourceChannelFilter(channel);
    qDebug() << "🔍 设置源通道过滤:" << (channel == -1 ? "ALL" : QString::number(channel));
}

/**
 * @brief 加载文件
 */
bool FramePlaybackCore::loadFile(const QString &filename)
{
    qDebug() << "📁 FramePlaybackCore 加载文件:" << filename;
    
    // 使用文件解析器解析文件
    QVector<CANFrame> frames;
    QString errorMsg;
    
    if (!FrameFileParser::parseFile(filename, frames, errorMsg)) {
        qDebug() << "❌ 文件解析失败:" << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }
    
    // 按时间戳排序
    std::sort(frames.begin(), frames.end(), [](const CANFrame &a, const CANFrame &b) {
        return a.timestamp < b.timestamp;
    });
    
    // 创建序列项
    PlaybackSequenceItem item;
    item.filename = filename;
    item.frames = frames;
    item.maxLoops = 1;
    item.currentLoopCount = 0;
    
    // 初始化ID过滤器（默认全部通过）
    for (const CANFrame &frame : frames) {
        if (!item.idFilters.contains(frame.id)) {
            item.idFilters[frame.id] = true;
        }
    }
    
    // 添加到序列列表
    addSequence(item);
    
    qDebug() << "✅ 文件加载成功，帧数:" << frames.size();
    return true;
}

/**
 * @brief 添加序列
 */
void FramePlaybackCore::addSequence(const PlaybackSequenceItem &item)
{
    m_sequences.append(item);
    qDebug() << "➕ 添加序列:" << item.filename << "帧数:" << item.frames.size();
}

/**
 * @brief 删除序列
 */
void FramePlaybackCore::removeSequence(int index)
{
    if (index >= 0 && index < m_sequences.size()) {
        QString filename = m_sequences[index].filename;
        m_sequences.removeAt(index);
        qDebug() << "🗑 删除序列:" << filename;
    }
}

/**
 * @brief 清空序列
 */
void FramePlaybackCore::clearSequences()
{
    m_sequences.clear();
    qDebug() << "🗑 清空所有序列";
}

/**
 * @brief 获取总帧数
 */
int FramePlaybackCore::totalFrames() const
{
    if (m_state.currentSeqIndex >= 0 && m_state.currentSeqIndex < m_sequences.size()) {
        return m_sequences[m_state.currentSeqIndex].frames.size();
    }
    return 0;
}

/**
 * @brief 播放
 */
bool FramePlaybackCore::play()
{
    if (m_sequences.isEmpty()) {
        emit errorOccurred("没有可播放的序列");
        return false;
    }
    
    if (m_currentDevice.isEmpty()) {
        emit errorOccurred("未设置设备");
        return false;
    }
    
    // 检查设备连接
    if (!ConnectionManager::instance()->isConnected(m_currentDevice)) {
        emit errorOccurred("设备未连接");
        return false;
    }
    
    if (m_state.isPlaying) {
        return true;
    }
    
    m_state.isPlaying = true;
    m_state.isPaused = false;
    
    // 启动当前序列
    startCurrentSequence();
    
    return true;
}

bool FramePlaybackCore::playFrames(const QVector<CANFrame> &frames)
{
    if (frames.isEmpty()) {
        emit errorOccurred("没有可播放的帧");
        return false;
    }
    
    if (m_currentDevice.isEmpty()) {
        emit errorOccurred("未设置设备");
        return false;
    }
    
    // 检查设备连接
    if (!ConnectionManager::instance()->isConnected(m_currentDevice)) {
        emit errorOccurred("设备未连接");
        return false;
    }
    
    if (m_state.isPlaying) {
        // 避免并发播放导致队列/Provider 状态错乱
        return false;
    }
    
    m_customFramesPlayback = true;
    m_customFrames = frames;                 // 保存用于循环子集回放
    m_customFramesLoop = m_state.loopSequence;
    m_customFramesLoopCount = 0;
    m_state.isPlaying = true;
    m_state.isPaused = false;
    m_state.currentFrameIndex = 0;
    
    // 创建 Provider（复制 frames，保证 provider 生命周期安全）
    MemoryFrameProvider *provider = new MemoryFrameProvider(m_customFrames);
    m_thread->setProvider(provider);
    
    // 配置线程
    m_thread->setDevice(m_currentDevice);
    m_thread->setSpeed(m_state.playbackSpeed);
    m_thread->setUseOriginalTiming(m_state.useOriginalTiming);
    m_thread->setStepIntervalMs(m_state.stepIntervalMs);
    
    // 设置通道映射
    m_thread->clearChannelMapping();
    QMapIterator<int, int> i(m_channelMap);
    while (i.hasNext()) {
        i.next();
        m_thread->setChannelMapping(i.key(), i.value());
    }
    
    m_thread->startPlayback();
    
    qDebug() << "▶ 开始播放选中帧子集";
    return true;
}

/**
 * @brief 启动当前序列
 */
void FramePlaybackCore::startCurrentSequence()
{
    if (m_state.currentSeqIndex >= m_sequences.size()) {
        m_state.currentSeqIndex = 0;
    }
    
    const PlaybackSequenceItem &seq = m_sequences[m_state.currentSeqIndex];
    
    // 创建 Provider
    // 注意：MemoryFrameProvider 会持有 seq.frames 的引用（隐式共享），所以 seq 不能被析构
    // 由于 m_sequences 是 FramePlaybackCore 的成员，这通常是安全的，除非在播放时修改了 m_sequences
    // 如果播放时修改了 sequences，可能会有风险
    // TODO: 考虑在 MemoryFrameProvider 中进行深拷贝，或者确保 sequences 线程安全
    
    MemoryFrameProvider *provider = new MemoryFrameProvider(seq.frames, seq.idFilters);
    m_thread->setProvider(provider);
    
    // 配置线程
    m_thread->setDevice(m_currentDevice);
    m_thread->setSpeed(m_state.playbackSpeed);
    m_thread->setUseOriginalTiming(m_state.useOriginalTiming);
    m_thread->setStepIntervalMs(m_state.stepIntervalMs);
    
    // 设置通道映射
    // 注意：m_thread 已经保存了映射表，但如果我们想确保最新，可以重新设置
    // 但 setChannelMapping 是增量的，所以应该在 Core 中维护一份完整的，每次播放前清除并重新设置？
    // 或者我们假设 Core 和 Thread 是一致的。
    // 为了保险，我们可以在 Core 中维护 map，每次 startCurrentSequence 时同步给 Thread
    m_thread->clearChannelMapping();
    QMapIterator<int, int> i(m_channelMap);
    while (i.hasNext()) {
        i.next();
        m_thread->setChannelMapping(i.key(), i.value());
    }
    
    m_thread->startPlayback();
    
    qDebug() << "▶ 开始播放序列:" << seq.filename;
}

/**
 * @brief 暂停
 */
void FramePlaybackCore::pause()
{
    if (m_state.isPlaying && !m_state.isPaused) {
        m_state.isPaused = true;
        m_thread->pausePlayback();
        qDebug() << "⏸ 暂停播放";
    }
}

/**
 * @brief 恢复
 */
void FramePlaybackCore::resume()
{
    if (m_state.isPaused) {
        m_state.isPaused = false;
        m_thread->resumePlayback();
        qDebug() << "▶ 继续播放";
    }
}

void FramePlaybackCore::stepForward()
{
    qDebug() << "⚠️ stepForward 暂不支持（独立线程模式）";
}

void FramePlaybackCore::stepBackward()
{
    qDebug() << "⚠️ stepBackward 暂不支持（独立线程模式）";
}

void FramePlaybackCore::setStepInterval(int ms)
{
    if (ms < 1) ms = 1;
    if (ms > 1000) ms = 1000;
    m_state.stepIntervalMs = ms;
    m_thread->setStepIntervalMs(ms);
    qDebug() << "⏱ 设置步进间隔:" << ms << "ms";
}

/**
 * @brief 停止
 */
void FramePlaybackCore::stop()
{
    m_customFramesPlayback = false;
    m_customFramesLoop = false;
    m_customFramesLoopCount = 0;
    m_customFrames.clear();
    m_thread->stopPlayback();
    m_state.isPlaying = false;
    m_state.isPaused = false;
    m_state.currentFrameIndex = 0;
    // 重置序列索引？通常停止后回到开头
    m_state.currentSeqIndex = 0;
    qDebug() << "⏹ 停止播放";
}

/**
 * @brief 设置倍速
 */
void FramePlaybackCore::setPlaybackSpeed(double speed)
{
    if (speed <= 0.0) speed = 1.0;
    if (speed > 5.0) speed = 5.0;
    m_state.playbackSpeed = speed;
    m_thread->setSpeed(speed);
    qDebug() << "⚡ 设置倍速:" << speed;
}

/**
 * @brief 设置使用原始时间戳
 */
void FramePlaybackCore::setUseOriginalTiming(bool use)
{
    m_state.useOriginalTiming = use;
    m_thread->setUseOriginalTiming(use);
}

/**
 * @brief 设置循环序列
 */
void FramePlaybackCore::setLoopSequence(bool loop)
{
    m_state.loopSequence = loop;
    qDebug() << "🔁 循环序列:" << loop;
}

/**
 * @brief 设置ID过滤
 */
void FramePlaybackCore::setIDFilter(uint32_t id, bool pass)
{
    // 更新当前序列的过滤器
    if (m_state.currentSeqIndex >= 0 && m_state.currentSeqIndex < m_sequences.size()) {
        m_sequences[m_state.currentSeqIndex].idFilters[id] = pass;
        
        // 如果正在播放，还需要通知 Provider 更新过滤器
        // 但 MemoryFrameProvider 是在 Thread 里的，我们需要通过 Thread 接口去更新
        // 目前 PlaybackThread 没有 updateFilter 接口
        // TODO: 给 PlaybackThread 加 updateFilter 接口
        // 暂时不支持动态修改过滤
    }
}

/**
 * @brief 线程进度回调
 */
void FramePlaybackCore::onThreadProgress(int current, int total)
{
    m_state.currentFrameIndex = current;
    emit progressUpdated(current, total);
}

/**
 * @brief 线程完成回调
 */
void FramePlaybackCore::onThreadFinished()
{
    if (!m_state.isPlaying) return; // 手动停止的情况

    // 自定义帧子集回放：直接结束，不参与序列列表的 loop/切换逻辑
    if (m_customFramesPlayback) {
        if (m_customFramesLoop) {
            // 循环子集：重新创建 Provider 并从头开始
            m_customFramesLoopCount++;
            m_state.isPaused = false;
            m_state.currentFrameIndex = 0;

            MemoryFrameProvider *provider = new MemoryFrameProvider(m_customFrames);
            m_thread->setProvider(provider);

            m_thread->setDevice(m_currentDevice);
            m_thread->setSpeed(m_state.playbackSpeed);
            m_thread->setUseOriginalTiming(m_state.useOriginalTiming);
            m_thread->setStepIntervalMs(m_state.stepIntervalMs);

            m_thread->clearChannelMapping();
            QMapIterator<int, int> i(m_channelMap);
            while (i.hasNext()) {
                i.next();
                m_thread->setChannelMapping(i.key(), i.value());
            }

            m_thread->startPlayback();
            return;
        }

        // 子集不循环：结束并通知
        m_customFramesPlayback = false;
        m_state.isPlaying = false;
        m_state.isPaused = false;
        m_state.currentFrameIndex = 0;
        m_state.currentSeqIndex = 0;
        emit playbackFinished();
        return;
    }
    
    // 当前序列播放完成，检查循环或下一个序列
    if (m_state.currentSeqIndex >= m_sequences.size()) return;
    
    PlaybackSequenceItem &seq = m_sequences[m_state.currentSeqIndex];
    seq.currentLoopCount++;
    
    if (seq.maxLoops == -1 || seq.currentLoopCount < seq.maxLoops) {
        // 继续循环当前序列
        qDebug() << "🔁 序列循环:" << seq.currentLoopCount;
        startCurrentSequence();
        return;
    }
    
    // 切换到下一个序列
    seq.currentLoopCount = 0;
    m_state.currentSeqIndex++;
    
    if (m_state.currentSeqIndex >= m_sequences.size()) {
        if (m_state.loopSequence) {
            m_state.currentSeqIndex = 0;
            qDebug() << "🔁 循环整个序列列表";
            startCurrentSequence();
        } else {
            stop();
            emit playbackFinished();
            qDebug() << "✅ 所有序列播放完成";
        }
    } else {
        qDebug() << "➡ 切换到下一个序列";
        startCurrentSequence();
    }
}

/**
 * @brief 线程错误回调
 */
void FramePlaybackCore::onThreadError(const QString &msg)
{
    stop();
    emit errorOccurred(msg);
}
