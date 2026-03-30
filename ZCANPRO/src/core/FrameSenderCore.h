/**
 * @file FrameSenderCore.h
 * @brief 帧发送核心逻辑 - 管理周期发送和定时器
 * @author CANAnalyzerPro Team
 * @date 2026-02-25
 */

#ifndef FRAMESENDERCORE_H
#define FRAMESENDERCORE_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QElapsedTimer>
#include <QVector>
#include <QMap>
#include "CANFrame.h"

/**
 * @brief 帧发送配置结构体
 */
struct FrameSendConfig
{
    bool enabled;           // 是否启用
    quint32 id;             // CAN ID
    quint8 length;          // 数据长度
    bool isExtended;        // 是否扩展帧
    QByteArray data;        // 数据字节
    int interval;           // 发送间隔（毫秒）
    int maxCount;           // 最大发送次数（-1表示无限）
    
    // 运行时状态
    int currentCount;       // 当前已发送次数
    quint64 lastSendTime;   // 上次发送时间（微秒）
    quint64 accumulatedTime;// 累积时间（用于补偿误差）
    
    FrameSendConfig()
        : enabled(true)
        , id(0x100)
        , length(8)
        , isExtended(false)
        , data(QByteArray(8, 0))
        , interval(100)
        , maxCount(-1)
        , currentCount(0)
        , lastSendTime(0)
        , accumulatedTime(0)
    {}
};

/**
 * @brief 帧发送核心逻辑类
 * 
 * 职责：
 * - 管理多个发送配置
 * - 精确定时发送
 * - 统计发送数据
 */
class FrameSenderCore : public QObject
{
    Q_OBJECT
    
public:
    explicit FrameSenderCore(QObject *parent = nullptr);
    ~FrameSenderCore();
    
    /**
     * @brief 设置要使用的设备
     * @param device 设备串口名称
     */
    void setDevice(const QString &device);
    
    /**
     * @brief 设置要使用的通道
     * @param channel 通道号（0=CAN0, 1=CAN1）
     */
    void setChannel(int channel);
    
    /**
     * @brief 添加发送配置
     * @param config 发送配置
     * @return 配置索引
     */
    int addConfig(const FrameSendConfig &config);
    
    /**
     * @brief 更新发送配置
     * @param index 配置索引
     * @param config 新的配置
     */
    void updateConfig(int index, const FrameSendConfig &config);
    
    /**
     * @brief 删除发送配置
     * @param index 配置索引
     */
    void removeConfig(int index);
    
    /**
     * @brief 清空所有配置
     */
    void clearConfigs();
    
    /**
     * @brief 获取配置数量
     * @return 配置数量
     */
    int configCount() const { return m_configs.size(); }
    
    /**
     * @brief 获取指定配置
     * @param index 配置索引
     * @return 配置引用
     */
    const FrameSendConfig& getConfig(int index) const;
    
    /**
     * @brief 启动所有已启用的发送
     */
    void startAll();
    
    /**
     * @brief 停止所有发送
     */
    void stopAll();
    
    /**
     * @brief 是否正在发送
     * @return 正在发送返回true
     */
    bool isRunning() const { return m_isRunning; }
    
    /**
     * @brief 获取总发送帧数
     * @return 总发送帧数
     */
    int totalSent() const { return m_totalSent; }
    
signals:
    /**
     * @brief 统计信息更新信号
     * @param totalSent 总发送帧数
     */
    void statisticsUpdated(int totalSent);
    
    /**
     * @brief 错误发生信号
     * @param message 错误消息
     */
    void errorOccurred(const QString &message);
    
private slots:
    /**
     * @brief 定时器回调
     */
    void onTimerTick();
    
private:
    /**
     * @brief 发送单个配置的帧
     * @param config 发送配置
     */
    void sendFrame(FrameSendConfig &config);
    
    // 成员变量
    QTimer *m_timer;                    // 定时器（1ms精度）
    QElapsedTimer m_elapsedTimer;       // 精确计时器
    QVector<FrameSendConfig> m_configs; // 发送配置列表
    QString m_currentDevice;            // 当前设备
    int m_currentChannel;               // 当前通道
    bool m_isRunning;                   // 是否正在运行
    int m_totalSent;                    // 总发送帧数
};

#endif // FRAMESENDERCORE_H
