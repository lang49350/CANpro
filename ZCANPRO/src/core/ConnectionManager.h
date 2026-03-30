/**
 * @file ConnectionManager.h
 * @brief 连接管理器 - 管理所有设备连接（单例）
 * @author CANAnalyzerPro Team
 * @date 2024-02-08
 */

#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QStringList>
#include <QThread>
#include "SerialConnection.h"
#include "CANFrame.h"

/**
 * @brief 连接管理器（单例模式）
 * 
 * 职责：
 * - 管理多个设备的物理连接
 * - 接收原始数据并解析 UCAN-FD 协议
 * - 发送数据到DataRouter进行路由
 * 
 * 使用示例：
 * @code
 * ConnectionManager *mgr = ConnectionManager::instance();
 * mgr->connectDevice("COM3", 2000000);
 * @endcode
 */
class ConnectionManager : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief 获取单例实例
     * @return ConnectionManager单例指针
     */
    static ConnectionManager* instance();
    
    /**
     * @brief 连接指定设备
     * @param port 串口名称（如"COM3"）
     * @param baudRate 波特率（默认2000000）
     * @return 成功返回true，失败返回false
     * 
     * @note 如果设备已连接，会先断开再重新连接
     * @warning 连接失败会发送errorOccurred信号
     */
    bool connectDevice(const QString &port, int baudRate = 2000000);
    
    /**
     * @brief 断开指定设备
     * @param port 串口名称
     */
    void disconnectDevice(const QString &port);
    
    /**
     * @brief 断开所有设备
     */
    void disconnectAll();
    
    /**
     * @brief 检查设备是否已连接
     * @param port 串口名称
     * @return 已连接返回true，否则返回false
     */
    bool isConnected(const QString &port) const;
    
    /**
     * @brief 获取已连接的设备列表
     * @return 设备串口名称列表
     */
    QStringList connectedDevices() const;
    
    /**
     * @brief 获取可用的串口列表
     * @return 可用串口名称列表（格式："COM3 - USB Serial Port"）
     */
    static QStringList availablePorts();
    
    /**
     * @brief 发送CAN帧到指定设备
     * @param port 串口名称
     * @param frame CAN帧数据
     * @return 成功返回true，失败返回false
     */
    bool sendFrame(const QString &port, const CANFrame &frame);
    bool sendFrameSync(const QString &port, const CANFrame &frame);
    bool sendFrameSyncImmediate(const QString &port, const CANFrame &frame);
    
    // 无锁高频发送接口
    void enqueueSendFrame(const QString &port, const CANFrame &frame);
    void enqueueSendFrames(const QString &port, const QVector<CANFrame> &frames);
    
    /**
     * @brief 获取指定设备的连接对象
     * @param port 串口名称
     * @return SerialConnection指针，如果不存在返回nullptr
     */
    SerialConnection* getConnection(const QString &port) const;
    
    /**
     * @brief 通道工作模式枚举
     */
    enum ChannelMode {
        MODE_NORMAL = 0,    // 正常模式（双向收发）
        MODE_RX_ONLY = 1,   // 只读模式（监听模式）
        MODE_TX_ONLY = 2,   // 只发模式（回放模式）
        MODE_DISABLED = -1  // 通道禁用
    };

    /**
     * @brief 检查模式是否冲突
     * @param port 串口名称
     * @param channel 通道号
     * @param mode 欲申请的模式
     * @param outReason 输出参数：冲突原因描述
     * @return 如果不冲突返回true，否则返回false
     */
    bool checkModeConflict(const QString &port, int channel, ChannelMode mode, QString &outReason) const;

    /**
     * @brief 申请设置通道模式
     * @param port 串口名称
     * @param channel 通道号
     * @param mode 申请的模式
     * @param requester 申请者指针（用于跟踪需求）
     * @return 最终确定的模式
     * 
     * @note 逻辑：如果多个申请者，MODE_NORMAL 优先级最高，
     *       MODE_RX_ONLY 和 MODE_TX_ONLY 同时存在时也会升级为 MODE_NORMAL。
     */
    ChannelMode requestChannelMode(const QString &port, int channel, ChannelMode mode, QObject *requester);

    /**
     * @brief 申请设置通道模式（同时指定 CAN-FD/2.0）
     * @param isFD 是否申请 CAN-FD 模式（true=CAN FD，false=CAN 2.0）
     */
    ChannelMode requestChannelMode(const QString &port, int channel, ChannelMode mode, QObject *requester, bool isFD);

    /**
     * @brief 释放通道模式申请
     * @param port 串口名称
     * @param channel 通道号
     * @param requester 申请者指针
     */
    void releaseChannelMode(const QString &port, int channel, QObject *requester);

    /**
     * @brief 获取通道当前确定的模式
     * @param port 串口名称
     * @param channel 通道号
     * @return 当前模式
     */
    ChannelMode currentChannelMode(const QString &port, int channel) const;

signals:
    /**
     * @brief 通道模式发生实际改变信号
     * @param port 设备名称
     * @param channel 通道号
     * @param mode 新模式
     */
    void channelModeChanged(const QString &port, int channel, ChannelMode mode);

    /**
     * @brief 接收到CAN帧信号
     * @param port 设备串口名称
     * @param frame CAN帧数据
     */
    void frameReceived(const QString &port, const CANFrame &frame);
    
    /**
     * @brief 批量接收CAN帧信号
     * @param port 设备串口名称
     * @param frames CAN帧数据列表
     */
    void framesReceived(const QString &port, const QVector<CANFrame> &frames);
    
    /**
     * @brief 设备连接成功信号
     * @param port 设备串口名称
     */
    void deviceConnected(const QString &port);
    
    /**
     * @brief 设备断开连接信号
     * @param port 设备串口名称
     */
    void deviceDisconnected(const QString &port);
    
    /**
     * @brief 错误发生信号
     * @param port 设备串口名称
     * @param error 错误描述
     */
    void errorOccurred(const QString &port, const QString &error);
    
private:
    // 私有构造函数（单例模式）
    explicit ConnectionManager(QObject *parent = nullptr);
    ~ConnectionManager();
    
    // 禁止拷贝和赋值
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;
    
    // 连接SerialConnection的信号
    void setupConnectionSignals(const QString &port, SerialConnection *connection);
    
    /**
     * @brief 重新评估并应用指定通道的模式
     */
    void reevaluateChannelMode(const QString &port, int channel);

    /** 设备未连接时的 WARNING 节流（避免 enqueue 高频路径刷屏） */
    void warnDeviceNotConnectedThrottled(const QString &port, const char *kind, const QString &message);
    void clearNotConnectedLogThrottle(const QString &port);
    
    // 成员变量
    static ConnectionManager *s_instance;           // 单例实例
    QMap<QString, SerialConnection*> m_connections; // 设备连接映射表
    QMap<QString, QThread*> m_ioThreads;            // IO线程映射表
    
    // 需求跟踪: 设备 -> 通道 -> (申请者 -> 模式)
    QMap<QString, QMap<int, QMap<QObject*, ChannelMode>>> m_channelRequirements;
    // 需求跟踪: 设备 -> 通道 -> (申请者 -> CAN 类型：FD/2.0)
    QMap<QString, QMap<int, QMap<QObject*, bool>>> m_fdRequirements;
    // 当前生效模式: 设备 -> 通道 -> 模式
    QMap<QString, QMap<int, ChannelMode>> m_activeModes;
    // 当前生效 CAN 类型: 设备 -> 通道 -> isFD
    QMap<QString, QMap<int, bool>> m_activeIsFD;

    /** port|kind -> 上次输出 WARNING 的 epoch ms */
    QMap<QString, qint64> m_lastNotConnectedWarnMs;
};

#endif // CONNECTIONMANAGER_H
