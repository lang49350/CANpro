#ifndef FRAMEPLAYBACKCORE_H
#define FRAMEPLAYBACKCORE_H

#include <QObject>
#include <QVector>
#include <QHash>
#include <QString>
#include <QMap>
#include "CANFrame.h"
#include "PlaybackThread.h"

/**
 * @brief 回放序列项
 */
struct PlaybackSequenceItem {
    QString filename;                   // 文件名
    QVector<CANFrame> frames;           // 帧数据
    QHash<uint32_t, bool> idFilters;    // ID过滤器（ID -> 是否通过）
    int maxLoops;                       // 最大循环次数（-1=无限）
    int currentLoopCount;               // 当前循环计数
    
    PlaybackSequenceItem()
        : maxLoops(1)
        , currentLoopCount(0)
    {}
};

/**
 * @brief 回放状态
 */
struct PlaybackState {
    bool isPlaying;                     // 是否正在播放
    bool isPaused;                      // 是否暂停
    int currentSeqIndex;                // 当前序列索引
    int currentFrameIndex;              // 当前帧索引
    
    // 回放设置
    bool useOriginalTiming;             // 使用原始时间戳
    bool loopSequence;                  // 循环整个序列
    double playbackSpeed;               // 回放倍速（1.0 = 正常）
    int stepIntervalMs;                 // 非原始时间戳模式下的帧间隔（毫秒）
    
    PlaybackState()
        : isPlaying(false)
        , isPaused(false)
        , currentSeqIndex(0)
        , currentFrameIndex(0)
        , useOriginalTiming(true)
        , loopSequence(false)
        , playbackSpeed(1.0)
        , stepIntervalMs(10)
    {}
};

/**
 * @brief 帧回放核心逻辑
 * 
 * 职责：
 * - 管理回放序列
 * - 控制回放线程
 * - 管理通道映射
 */
class FramePlaybackCore : public QObject
{
    Q_OBJECT
    
public:
    explicit FramePlaybackCore(QObject *parent = nullptr);
    ~FramePlaybackCore();
    
    /**
     * @brief 设置设备
     */
    void setDevice(const QString &device);
    
    /**
     * @brief 设置默认输出通道
     * @param channel 目标通道
     */
    void setChannel(int channel);

    /**
     * @brief 设置通道映射
     * @param source 源通道
     * @param target 目标通道
     */
    void setChannelMapping(int source, int target);
    void clearChannelMapping();

    /**
     * @brief 设置源通道过滤
     * @param channel 源通道号（-1 表示全部）
     */
    void setSourceChannelFilter(int channel);
    
    /**
     * @brief 加载文件
     * @param filename 文件路径
     * @return 是否成功
     */
    bool loadFile(const QString &filename);
    
    /**
     * @brief 序列管理
     */
    void addSequence(const PlaybackSequenceItem &item);
    void removeSequence(int index);
    void clearSequences();
    int sequenceCount() const { return m_sequences.size(); }
    
    /**
     * @brief 回放控制
     */
    bool play();
    
    /**
     * @brief 使用给定帧进行一次性回放（不依赖已加载的序列列表）
     * 
     * @note 时间控制完全复用当前 m_state.useOriginalTiming / m_state.stepIntervalMs / m_state.playbackSpeed
     */
    bool playFrames(const QVector<CANFrame> &frames);
    void pause();
    void resume();
    void stop();
    void stepForward();
    void stepBackward();
    
    /**
     * @brief 回放设置
     */
    void setPlaybackSpeed(double speed);
    void setStepInterval(int ms);
    void setUseOriginalTiming(bool use);
    void setLoopSequence(bool loop);
    // TODO: ID过滤需要传递给 Provider 或 Thread
    void setIDFilter(uint32_t id, bool pass);
    
    /**
     * @brief 获取状态
     */
    bool isPlaying() const { return m_state.isPlaying; }
    bool isPaused() const { return m_state.isPaused; }
    int currentFrameIndex() const { return m_state.currentFrameIndex; }
    int totalFrames() const;
    
signals:
    /**
     * @brief 进度更新信号
     * @param current 当前帧索引
     * @param total 总帧数
     */
    void progressUpdated(int current, int total);
    
    /**
     * @brief 回放完成信号
     */
    void playbackFinished();
    
    /**
     * @brief 错误信号
     * @param message 错误消息
     */
    void errorOccurred(const QString &message);
    
private slots:
    void onThreadProgress(int current, int total);
    void onThreadFinished();
    void onThreadError(const QString &msg);

private:
    void startCurrentSequence();

    PlaybackThread *m_thread;           // 回放线程
    QVector<PlaybackSequenceItem> m_sequences;  // 序列列表
    PlaybackState m_state;              // 回放状态
    
    QString m_currentDevice;            // 当前设备
    QMap<int, int> m_channelMap;        // 通道映射
    
    // 自定义子集回放状态：用于右键“回放选中帧（子集）”
    bool m_customFramesPlayback = false;
    QVector<CANFrame> m_customFrames;
    bool m_customFramesLoop = false;   // 对子集生效的循环开关
    int m_customFramesLoopCount = 0;   // 当前循环次数
};

#endif // FRAMEPLAYBACKCORE_H
