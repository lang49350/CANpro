/**
 * @file FrameSenderCore.cpp
 * @brief 帧发送核心逻辑实现
 * @author CANAnalyzerPro Team
 * @date 2026-02-25
 */

#include "FrameSenderCore.h"
#include "ConnectionManager.h"
#include <QDebug>

/**
 * @brief 构造函数
 */
FrameSenderCore::FrameSenderCore(QObject *parent)
    : QObject(parent)
    , m_timer(nullptr)
    , m_currentDevice("")
    , m_currentChannel(0)
    , m_isRunning(false)
    , m_totalSent(0)
{
    // 创建定时器（1ms精度）
    m_timer = new QTimer(this);
    m_timer->setInterval(1);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &FrameSenderCore::onTimerTick);
    
    qDebug() << "✅ FrameSenderCore 创建成功";
}

/**
 * @brief 析构函数
 */
FrameSenderCore::~FrameSenderCore()
{
    stopAll();
    qDebug() << "🗑 FrameSenderCore 销毁";
}

/**
 * @brief 设置设备
 */
void FrameSenderCore::setDevice(const QString &device)
{
    m_currentDevice = device;
    qDebug() << "📡 FrameSenderCore 设置设备:" << device;
}

/**
 * @brief 设置通道
 */
void FrameSenderCore::setChannel(int channel)
{
    m_currentChannel = channel;
    qDebug() << "📡 FrameSenderCore 设置通道:" << channel;
}

/**
 * @brief 添加发送配置
 */
int FrameSenderCore::addConfig(const FrameSendConfig &config)
{
    m_configs.append(config);
    int index = m_configs.size() - 1;
    qDebug() << "➕ 添加发送配置，索引:" << index << "ID:" << QString::number(config.id, 16);
    return index;
}

/**
 * @brief 更新发送配置
 */
void FrameSenderCore::updateConfig(int index, const FrameSendConfig &config)
{
    if (index >= 0 && index < m_configs.size()) {
        m_configs[index] = config;
        qDebug() << "📝 更新发送配置，索引:" << index;
    }
}

/**
 * @brief 删除发送配置
 */
void FrameSenderCore::removeConfig(int index)
{
    if (index >= 0 && index < m_configs.size()) {
        m_configs.removeAt(index);
        qDebug() << "🗑 删除发送配置，索引:" << index;
    }
}

/**
 * @brief 清空所有配置
 */
void FrameSenderCore::clearConfigs()
{
    m_configs.clear();
    qDebug() << "🗑 清空所有发送配置";
}

/**
 * @brief 获取指定配置
 */
const FrameSendConfig& FrameSenderCore::getConfig(int index) const
{
    static FrameSendConfig emptyConfig;
    if (index >= 0 && index < m_configs.size()) {
        return m_configs[index];
    }
    return emptyConfig;
}

/**
 * @brief 启动所有已启用的发送
 */
void FrameSenderCore::startAll()
{
    if (m_currentDevice.isEmpty()) {
        qDebug() << "⚠️ 未设置设备";
        emit errorOccurred("未设置设备");
        return;
    }
    
    // 检查设备连接
    if (!ConnectionManager::instance()->isConnected(m_currentDevice)) {
        qDebug() << "❌ 设备未连接:" << m_currentDevice;
        emit errorOccurred("设备未连接: " + m_currentDevice);
        return;
    }
    
    // 检查是否有启用的配置
    bool hasEnabled = false;
    for (const FrameSendConfig &config : m_configs) {
        if (config.enabled) {
            hasEnabled = true;
            break;
        }
    }
    
    if (!hasEnabled) {
        qDebug() << "⚠️ 没有启用的发送配置";
        emit errorOccurred("没有启用的发送配置");
        return;
    }
    
    // 重置所有配置的运行时状态
    for (FrameSendConfig &config : m_configs) {
        config.currentCount = 0;
        config.lastSendTime = 0;
        config.accumulatedTime = 0;
    }
    
    m_isRunning = true;
    m_totalSent = 0;
    m_elapsedTimer.start();
    m_timer->start();
    
    qDebug() << "▶ 开始发送";
}

/**
 * @brief 停止所有发送
 */
void FrameSenderCore::stopAll()
{
    m_timer->stop();
    m_isRunning = false;
    
    qDebug() << "⏹ 停止发送，总发送:" << m_totalSent << "帧";
}

/**
 * @brief 定时器回调
 */
void FrameSenderCore::onTimerTick()
{
    if (!m_isRunning) return;
    
    // 获取经过的时间（微秒）
    quint64 elapsed = m_elapsedTimer.nsecsElapsed() / 1000;
    m_elapsedTimer.start();
    
    // 遍历所有配置
    for (FrameSendConfig &config : m_configs) {
        if (!config.enabled) continue;
        
        // 检查是否达到最大发送次数
        if (config.maxCount != -1 && config.currentCount >= config.maxCount) {
            continue;
        }
        
        // 累积时间
        config.accumulatedTime += elapsed;
        
        // 计算应该发送的次数
        quint64 intervalUs = config.interval * 1000;  // 转换为微秒
        if (intervalUs == 0) intervalUs = 1000;  // 最小1ms
        
        int sendCount = config.accumulatedTime / intervalUs;
        
        // 发送帧
        for (int i = 0; i < sendCount; i++) {
            // 检查是否达到最大发送次数
            if (config.maxCount != -1 && config.currentCount >= config.maxCount) {
                break;
            }
            
            sendFrame(config);
            config.currentCount++;
            m_totalSent++;
        }
        
        // 减去已发送的时间
        config.accumulatedTime -= sendCount * intervalUs;
    }
    
    // 每100ms更新一次统计信息
    static int tickCount = 0;
    tickCount++;
    if (tickCount >= 100) {
        emit statisticsUpdated(m_totalSent);
        tickCount = 0;
    }
}

/**
 * @brief 发送单个配置的帧
 */
void FrameSenderCore::sendFrame(FrameSendConfig &config)
{
    // 构造CAN帧
    CANFrame frame;
    frame.id = config.id;
    frame.length = config.length;
    frame.isExtended = config.isExtended;
    frame.channel = m_currentChannel;
    frame.data = config.data;
    frame.isReceived = false;
    frame.timestamp = QDateTime::currentMSecsSinceEpoch() * 1000;  // 微秒
    
    // 发送帧
    ConnectionManager::instance()->enqueueSendFrame(m_currentDevice, frame);
}
