#ifndef PLAYBACKTHREAD_H
#define PLAYBACKTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QMap>
#include <QAtomicInt>
#include "FrameProvider.h"
#include "CANFrame.h"

/**
 * @brief 回放线程
 * 
 * 负责在独立线程中执行高精度的回放循环
 */
class PlaybackThread : public QThread
{
    Q_OBJECT

public:
    explicit PlaybackThread(QObject *parent = nullptr);
    ~PlaybackThread();

    /**
     * @brief 设置数据提供者
     * @param provider 数据提供者（所有权转移给线程）
     */
    void setProvider(IFrameProvider *provider);

    /**
     * @brief 设置回放设备
     * @param device 设备名称（如 COM3）
     */
    void setDevice(const QString &device);

    /**
     * @brief 设置通道映射
     * @param source 源通道
     * @param target 目标通道
     */
    void setChannelMapping(int source, int target);
    void clearChannelMapping();

    /**
     * @brief 设置源通道过滤
     * @param channel 源通道号（-1 表示不过滤）
     */
    void setSourceChannelFilter(int channel);

    /**
     * @brief 设置回放速度
     * @param speed 倍速 (1.0 = 正常速度)
     */
    void setSpeed(double speed);
    void setUseOriginalTiming(bool useOriginalTiming);
    void setStepIntervalMs(int stepIntervalMs);

    /**
     * @brief 控制命令
     */
    void startPlayback();
    void pausePlayback();
    void resumePlayback();
    void stopPlayback();

    /**
     * @brief 状态查询
     */
    bool isPlaying() const;
    bool isPaused() const;

protected:
    void run() override;

signals:
    void progressUpdated(int current, int total);
    void playbackFinished();
    void errorOccurred(const QString &msg);

private:
    IFrameProvider *m_provider;
    QString m_device;
    QMap<int, int> m_channelMap;
    int m_sourceChannelFilter;
    double m_speed;
    bool m_useOriginalTiming;
    int m_stepIntervalMs;
    
    // 线程同步
    QMutex m_mutex;
    QWaitCondition m_cond;
    QAtomicInt m_state; // 0=Stopped, 1=Running, 2=Paused
    bool m_abort;

    // 辅助函数
    void highPrecisionSleep(qint64 microseconds);
};

#endif // PLAYBACKTHREAD_H
