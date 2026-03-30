/**
 * @file DataRouter.h
 * @brief 数据路由器 - 管理窗口订阅关系并分发数据（单例）
 * @author CANAnalyzerPro Team
 * @date 2024-02-08
 */

#ifndef DATAROUTER_H
#define DATAROUTER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include "CANFrame.h"

// 前置声明
class QWidget;

/**
 * @brief 数据路由器（单例模式）
 * 
 * 职责：
 * - 管理窗口的订阅关系（哪个窗口订阅哪个设备的哪个通道）
 * - 接收ConnectionManager的数据
 * - 根据订阅关系分发数据到对应窗口
 * 
 * 使用示例：
 * @code
 * DataRouter *router = DataRouter::instance();
 * router->subscribe(widget, "COM3", 0);  // 订阅COM3的CAN0通道
 * @endcode
 */
class DataRouter : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief 获取单例实例
     * @return DataRouter单例指针
     */
    static DataRouter* instance();
    
    /**
     * @brief 订阅指定设备的指定通道
     * @param widget 订阅的窗口指针
     * @param device 设备串口名称（如"COM3"）
     * @param channel 通道号（-1=ALL, 0=CAN0, 1=CAN1, ...）
     * 
     * @note 如果窗口已有订阅，会先取消旧订阅再添加新订阅
     */
    void subscribe(QWidget *widget, const QString &device, int channel);
    
    /**
     * @brief 取消窗口的订阅
     * @param widget 窗口指针
     */
    void unsubscribe(QWidget *widget);
    
    /**
     * @brief 路由数据帧到订阅的窗口
     * @param device 设备串口名称
     * @param frame CAN帧数据
     * 
     * @note 此函数由ConnectionManager调用
     */
    void routeFrame(const QString &device, const CANFrame &frame);
    
    /**
     * @brief 批量路由数据帧
     * @param device 设备串口名称
     * @param frames CAN帧数据列表
     */
    void routeFrames(const QString &device, const QVector<CANFrame> &frames);
    
    /**
     * @brief 获取窗口的订阅信息
     * @param widget 窗口指针
     * @param device 输出参数：设备名称
     * @param channel 输出参数：通道号
     * @return 如果窗口有订阅返回true，否则返回false
     */
    bool getSubscription(QWidget *widget, QString &device, int &channel) const;
    
    /**
     * @brief 检查设备是否还有订阅者
     * @param device 设备串口名称
     * @return 如果有订阅者返回true，否则返回false
     */
    bool hasSubscribers(const QString &device) const;
    
signals:
    /**
     * @brief 数据帧路由到窗口信号
     * @param widget 目标窗口
     * @param frame CAN帧数据
     */
    void frameRoutedToWidget(QWidget *widget, const CANFrame &frame);
    
    /**
     * @brief 批量数据帧路由到窗口信号
     * @param widget 目标窗口
     * @param frames CAN帧数据列表
     */
    void framesRoutedToWidget(QWidget *widget, const QVector<CANFrame> &frames);
    
private:
    // 私有构造函数（单例模式）
    explicit DataRouter(QObject *parent = nullptr);
    ~DataRouter();
    
    // 禁止拷贝和赋值
    DataRouter(const DataRouter&) = delete;
    DataRouter& operator=(const DataRouter&) = delete;
    
    /**
     * @brief 订阅信息结构体
     */
    struct Subscription {
        QWidget *widget;    // 订阅的窗口
        QString device;     // 设备串口名称（如"COM3"）
        int channel;        // 通道号（-1=ALL, 0=CAN0, 1=CAN1, ...）
        int bufferIndex;
        
        Subscription(QWidget *w, const QString &d, int c)
            : widget(w), device(d), channel(c), bufferIndex(-1) {}
    };
    
    // 🚀 阶段2优化：预分配窗口缓冲区（避免QMap动态分配）
    struct WidgetBuffer {
        QWidget *widget;
        QVector<CANFrame> frames;
        int lastRouteToken;
        
        WidgetBuffer(QWidget *w = nullptr) : widget(w), lastRouteToken(0) {
            frames.reserve(500);  // 预分配500帧容量
        }
    };
    
    /**
     * @brief 检查帧是否匹配订阅条件
     * @param sub 订阅信息
     * @param frame CAN帧数据
     * @return 匹配返回true，否则返回false
     */
    bool matchesSubscription(const Subscription &sub, const CANFrame &frame) const;
    
    /**
     * @brief 重建窗口缓冲区（当订阅变化时调用）
     */
    void rebuildWidgetBuffers();
    
    // 成员变量
    static DataRouter *s_instance;      // 单例实例
    QList<Subscription> m_subscriptions; // 订阅列表
    QVector<WidgetBuffer> m_widgetBuffers;  // 🚀 阶段2优化：预分配缓冲区池
    QHash<QString, QVector<int>> m_deviceAllSubscriptionIndices;
    QHash<QString, QHash<int, QVector<int>>> m_deviceChannelSubscriptionIndices;
    QHash<QWidget*, int> m_widgetBufferIndex;
    QVector<int> m_touchedBufferIndices;
    int m_routeToken;
};

#endif // DATAROUTER_H
