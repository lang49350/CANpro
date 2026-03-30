/**
 * @file CANFrameTableModel.h
 * @brief CAN帧表格数据模型 - 支持虚拟滚动
 * @author CANAnalyzerPro Team
 * @date 2024-02-10
 */

#ifndef CANFRAMETABLEMODEL_H
#define CANFRAMETABLEMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <deque>
#include <QDateTime>
#include <QTimer>
#include "CANFrame.h"
#include "FilterManager.h"
#include "IDFilterRule.h"

/**
 * @brief CAN帧表格数据模型
 * 
 * 特点：
 * - 支持虚拟滚动（可处理百万级数据）
 * - 批量插入优化（避免频繁UI更新）
 * - 异步筛选（不阻塞UI）
 * - 内存高效（只存储原始数据）
 * - 性能优化：使用std::deque和绝对索引管理，避免O(N)删除开销
 */
class CANFrameTableModel : public QAbstractTableModel
{
    Q_OBJECT
    
public:
    enum Column {
        COL_SEQ = 0,      // 序号
        COL_TIME,         // 时间
        COL_CHANNEL,      // 源通道
        COL_ID,           // 帧ID
        COL_TYPE,         // CAN类型
        COL_DIR,          // 方向
        COL_LEN,          // 长度
        COL_DATA,         // 数据
        COL_COUNT
    };
    
    explicit CANFrameTableModel(QObject *parent = nullptr);
    ~CANFrameTableModel();
    
    // QAbstractTableModel必须实现的方法
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    // 数据操作
    void addFrame(const CANFrame &frame);
    void addFrames(const QVector<CANFrame> &frames);
    void clearFrames();
    QVector<CANFrame> getAllFrames() const;
    int totalFrames() const { return m_ringSize; }
    
    // 环形缓冲区访问
    const CANFrame& getFrameAt(int index) const;
    const CANFrame& getFrameByRingIndex(int ringIndex) const { return m_ringBuffer[ringIndex]; }
    int getCapacity() const { return m_ringCapacity; }
    quint64 getTotalFramesReceived() const { return m_totalFramesReceived; }
    
    // 格式设置
    void setTimeFormat(int format) { m_timeFormat = format; }
    void setDataFormat(int format) { m_dataFormat = format; }
    void setIDFormat(int format) { m_idFormat = format; }
    void setStartTime(const QDateTime &startTime) { m_startTime = startTime; }
    
    int getTimeFormat() const { return m_timeFormat; }
    int getDataFormat() const { return m_dataFormat; }
    int getIDFormat() const { return m_idFormat; }
    
    // 筛选控制
    void setTypeFilter(int type);
    void setDirectionFilter(int dir);
    void setDataFilter(const QString &filter);
    void setIDFilter(const QString &id);
    void setLengthFilter(const QString &length);
    void setChannelFilter(int channel);
    void setSeqFilter(const QString &seq);
    void setTimeFilter(const QString &time);
    
    /**
     * @brief 设置ID过滤规则（高级模式）
     * @param rule ID过滤规则
     */
    void setIDFilterRule(const IDFilterRule &rule);
    
    /**
     * @brief 获取ID过滤规则
     * @return ID过滤规则
     */
    IDFilterRule getIDFilterRule() const;
    
    // 获取筛选条件
    int getTypeFilter() const;
    int getDirectionFilter() const;
    QString getDataFilter() const;
    
signals:
    void framesReceived(const QVector<CANFrame> &frames);  // 批量信号
    
private slots:
    void flushPendingFrames();  // 批量插入待处理的帧
    
private:
    QString formatTime(quint64 timestamp) const;
    QString formatID(quint32 id) const;
    QString formatData(const QByteArray &data) const;
    QString formatType(const CANFrame &frame) const;
    QString formatDirection(bool isReceived) const;
    
    void applyFilters();  // 重新应用筛选
    void cleanupInvalidIndices(); // 清理失效的索引
    
    // 环形缓冲区实现
    CANFrame* m_ringBuffer;                 // 环形缓冲区
    int m_ringCapacity;                     // 缓冲区容量
    int m_ringHead;                         // 写指针
    int m_ringTail;                         // 读指针
    int m_ringSize;                         // 当前大小
    
    // 优化：使用deque存储绝对帧序号，支持O(1)头部删除
    std::deque<quint64> m_filteredIndices;  // 筛选后的绝对索引
    QVector<CANFrame> m_pendingFrames;      // 待插入的帧（批量处理）
    
    FilterManager *m_filterManager;         // 筛选管理器
    QTimer *m_flushTimer;                   // 批量插入定时器
    
    // 格式设置
    int m_timeFormat;                       // 时间格式: 0=相对时间, 1=绝对时间
    int m_dataFormat;                       // 数据格式: 0=十六进制, 1=十进制
    int m_idFormat;                         // ID格式: 0=十六进制, 1=十进制
    QDateTime m_startTime;                  // 开始时间
    quint64 m_startTimeUs;                  // 开始时间（微秒，用于快速计算）
    
    quint64 m_totalFramesReceived;          // 总接收帧数（绝对计数器）
    
    // 性能优化：HEX查表
    static const char* HEX_TABLE[256];
    
    static const int MAX_FRAMES = 300000;   // 最多保存30万条
    static const int FLUSH_INTERVAL = 100;  // 批量插入间隔（毫秒）- 优化：从20ms提升到100ms，减少UI刷新频率
};

#endif // CANFRAMETABLEMODEL_H
