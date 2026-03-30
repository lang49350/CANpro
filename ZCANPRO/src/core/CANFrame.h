#ifndef CANFRAME_H
#define CANFRAME_H

#include <QObject>
#include <QByteArray>
#include <QDateTime>

/**
 * @brief CAN帧数据结构
 * 
 * 支持标准CAN和CAN-FD帧格式
 */
class CANFrame
{
public:
    CANFrame();
    ~CANFrame();
    
    // CAN帧属性
    quint32 id;                 // CAN ID (11位标准帧或29位扩展帧)
    QByteArray data;            // 数据字节 (最多64字节，CAN-FD)
    quint64 timestamp;          // 时间戳 (微秒)
    quint8 length;              // 数据长度
    
    // 标志位
    bool isExtended;            // 是否扩展帧
    bool isRemote;              // 是否远程帧 (RTR)
    bool isCanFD;               // 是否CAN-FD帧
    bool isBRS;                 // 是否位速率切换 (Bit Rate Switch)
    bool isESI;                 // 是否错误状态指示 (Error State Indicator)
    bool isError;               // 是否错误帧
    bool isReceived;            // true=接收, false=发送
    
    // 辅助信息
    quint8 channel;             // 通道号 (0-3)
    QString direction;          // 方向 ("接收" 或 "发送")
    QString type;               // 类型 ("标准帧", "扩展帧", "远程帧", "CAN-FD")
    
    // 工具函数
    QString getIdString() const;        // 获取ID字符串 (十六进制)
    QString getDataString() const;      // 获取数据字符串 (十六进制，空格分隔)
    QString getTimestampString() const; // 获取时间戳字符串 (秒.微秒)
    QString getTypeString() const;      // 获取类型字符串
    QString getDirectionString() const; // 获取方向字符串
    
    // CANFD相关工具函数
    static quint8 dlcToLength(quint8 dlc);  // DLC转换为实际字节数
    static quint8 lengthToDLC(quint8 len);  // 实际字节数转换为DLC
    
    // 验证帧有效性
    bool isValid() const;
    
    // 复制构造和赋值
    CANFrame(const CANFrame &other);
    CANFrame& operator=(const CANFrame &other);
};

#endif // CANFRAME_H
