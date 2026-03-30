/**
 * @file CANFrameTableModel.cpp
 * @brief CAN帧表格数据模型实现
 * @author CANAnalyzerPro Team
 * @date 2024-02-10
 */

#include "CANFrameTableModel.h"
#include <QColor>

// 调试日志开关（生产环境设置为0）
#define CANFRAME_VERBOSE_LOGGING 0

#if CANFRAME_VERBOSE_LOGGING
#include <QDebug>
#define CANFRAME_LOG_DEBUG(msg) qDebug() << msg
#else
#define CANFRAME_LOG_DEBUG(msg)
#endif

// ==================== 性能优化：HEX查表 ====================
// 预计算0-255的十六进制字符串，避免运行时转换
const char* CANFrameTableModel::HEX_TABLE[256] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B", "0C", "0D", "0E", "0F",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1A", "1B", "1C", "1D", "1E", "1F",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", "3C", "3D", "3E", "3F",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F",
    "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", "6E", "6F",
    "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7A", "7B", "7C", "7D", "7E", "7F",
    "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F",
    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF",
    "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF",
    "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF",
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF",
    "E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF",
    "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB", "FC", "FD", "FE", "FF"
};

CANFrameTableModel::CANFrameTableModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_ringBuffer(nullptr)
    , m_ringCapacity(MAX_FRAMES)
    , m_ringHead(0)
    , m_ringTail(0)
    , m_ringSize(0)
    , m_filterManager(new FilterManager())
    , m_flushTimer(new QTimer(this))
    , m_timeFormat(0)
    , m_dataFormat(0)
    , m_idFormat(0)
    , m_startTime(QDateTime::currentDateTime())
    , m_startTimeUs(QDateTime::currentMSecsSinceEpoch() * 1000)  // 微秒
    , m_totalFramesReceived(0)
{
    // 分配环形缓冲区（固定大小）
    m_ringBuffer = new CANFrame[m_ringCapacity];
    // std::deque 不需要预分配大小，它会根据需要动态管理内存块
    
    // 设置批量插入定时器
    m_flushTimer->setInterval(FLUSH_INTERVAL);
    m_flushTimer->setSingleShot(true);
    connect(m_flushTimer, &QTimer::timeout, this, &CANFrameTableModel::flushPendingFrames);
    
    CANFRAME_LOG_DEBUG("✅ CANFrameTableModel 创建（环形缓冲区容量:" << m_ringCapacity << "）");
}

CANFrameTableModel::~CANFrameTableModel()
{
    delete m_filterManager;
    delete[] m_ringBuffer;
    CANFRAME_LOG_DEBUG("🗑 CANFrameTableModel 销毁");
}

// ==================== QAbstractTableModel 必须实现的方法 ====================

int CANFrameTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return (int)m_filteredIndices.size();
}

int CANFrameTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return COL_COUNT;
}

QVariant CANFrameTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= (int)m_filteredIndices.size()) {
        return QVariant();
    }
    
    // 1. 获取绝对序号 (O(1) 随机访问)
    quint64 absIndex = m_filteredIndices[index.row()];
    
    // 2. 检查索引有效性
    // 计算当前缓冲区里最早的有效绝对序号
    quint64 oldestValid = 0;
    if (m_totalFramesReceived > (quint64)m_ringCapacity) {
        oldestValid = m_totalFramesReceived - m_ringCapacity;
    }
    
    if (absIndex < oldestValid) {
        // 已失效（被覆盖），返回空数据
        return QVariant();
    }
    
    // 3. 映射回环形缓冲区索引
    int ringIndex = absIndex % m_ringCapacity;
    const CANFrame &frame = m_ringBuffer[ringIndex];
    
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case COL_SEQ:
                return (qulonglong)(absIndex + 1);  // 序号从1开始
            case COL_TIME:
                return formatTime(frame.timestamp);
            case COL_CHANNEL:
                return frame.channel;
            case COL_ID:
                return formatID(frame.id);
            case COL_TYPE:
                return formatType(frame);
            case COL_DIR:
                return formatDirection(frame.isReceived);
            case COL_LEN:
                return frame.length;
            case COL_DATA:
                return formatData(frame.data);
        }
    }
    else if (role == Qt::TextAlignmentRole) {
        // 数字列右对齐，其他左对齐
        if (index.column() == COL_SEQ || index.column() == COL_LEN || index.column() == COL_CHANNEL) {
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        }
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    }
    else if (role == Qt::ForegroundRole) {
        // 根据方向设置颜色
        if (frame.isReceived) {
            return QColor(0, 100, 0);  // 接收：深绿色
        } else {
            return QColor(0, 0, 150);  // 发送：深蓝色
        }
    }
    
    return QVariant();
}

QVariant CANFrameTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    
    if (orientation == Qt::Horizontal) {
        switch (section) {
            case COL_SEQ: return "序号";
            case COL_TIME: return "时间标识";
            case COL_CHANNEL: return "源通道";
            case COL_ID: return "帧ID";
            case COL_TYPE: return "CAN类型";
            case COL_DIR: return "方向";
            case COL_LEN: return "长度";
            case COL_DATA: return "数据";
        }
    }
    
    return QVariant();
}

// ==================== 数据操作 ====================

void CANFrameTableModel::addFrame(const CANFrame &frame)
{
    // 添加到待处理队列
    m_pendingFrames.append(frame);
    
    // 启动定时器（批量处理）
    if (!m_flushTimer->isActive()) {
        m_flushTimer->start();
    }
}

void CANFrameTableModel::addFrames(const QVector<CANFrame> &frames)
{
    if (frames.isEmpty()) {
        return;
    }
    
    // 添加到待处理队列
    m_pendingFrames.append(frames);
    
    // 启动定时器（批量处理）
    if (!m_flushTimer->isActive()) {
        m_flushTimer->start();
    }
}

void CANFrameTableModel::flushPendingFrames()
{
    if (m_pendingFrames.isEmpty()) {
        return;
    }
    
    // 收集通过筛选的绝对帧序号
    // 使用 std::vector 临时存储以避免频繁调用 beginInsertRows
    std::vector<quint64> newIndices;
    newIndices.reserve(m_pendingFrames.size());
    
    for (const CANFrame &frame : m_pendingFrames) {
        // 1. 存入环形缓冲区
        m_ringBuffer[m_ringHead] = frame;
        m_ringHead = (m_ringHead + 1) % m_ringCapacity;
        
        // 更新环形缓冲区状态
        if (m_ringSize < m_ringCapacity) {
            m_ringSize++;
        } else {
            // 缓冲区满，覆盖最旧的帧
            m_ringTail = (m_ringTail + 1) % m_ringCapacity;
        }
        
        // 2. 记录绝对序号
        quint64 absoluteIndex = m_totalFramesReceived++;
        
        // 计算相对时间
        qint64 relativeTime = m_startTime.msecsTo(QDateTime::fromMSecsSinceEpoch(frame.timestamp / 1000)) * 1000;
        
        // 3. 判断是否通过筛选
        if (m_filterManager->shouldDisplayFrame(frame, m_totalFramesReceived, relativeTime)) {
            newIndices.push_back(absoluteIndex);
        }
    }
    
    // 4. 清理失效的索引 (O(1) 关键优化)
    cleanupInvalidIndices();
    
    // 批量插入到显示列表
    if (!newIndices.empty()) {
        int first = (int)m_filteredIndices.size();
        int last = first + (int)newIndices.size() - 1;
        
        beginInsertRows(QModelIndex(), first, last);
        for (quint64 idx : newIndices) {
            m_filteredIndices.push_back(idx);
        }
        endInsertRows();
    }
    
    // 批量发送信号
    if (!m_pendingFrames.isEmpty()) {
        emit framesReceived(m_pendingFrames);
    }
    
    m_pendingFrames.clear();
}

void CANFrameTableModel::cleanupInvalidIndices()
{
    // 计算当前缓冲区里最早的有效绝对序号
    quint64 oldestValid = 0;
    if (m_totalFramesReceived > (quint64)m_ringCapacity) {
        oldestValid = m_totalFramesReceived - m_ringCapacity;
    }

    // 先计算需要删除多少个（不实际删除）
    int removedCount = 0;
    while (removedCount < (int)m_filteredIndices.size() && 
           m_filteredIndices[removedCount] < oldestValid) {
        removedCount++;
    }

    // 如果有需要删除的，按照Qt规范：先通知，再删除
    if (removedCount > 0) {
        // 1. 先通知 View 即将删除行（Qt规范要求）
        beginRemoveRows(QModelIndex(), 0, removedCount - 1);
        
        // 2. 实际删除数据（O(1) 操作）
        for (int i = 0; i < removedCount; i++) {
            m_filteredIndices.pop_front();  // O(1)!
        }
        
        // 3. 通知 View 删除完成
        endRemoveRows();
        
#if CANFRAME_VERBOSE_LOGGING
        // 调试日志（可选，低频输出）
        static int logCount = 0;
        if (logCount++ % 100 == 0) {
            qDebug() << "🗑 高效清理: 移除了" << removedCount << "个失效索引 (O(1)操作)";
        }
#endif
    }
}

void CANFrameTableModel::clearFrames()
{
    beginResetModel();
    m_ringHead = 0;
    m_ringTail = 0;
    m_ringSize = 0;
    m_filteredIndices.clear();
    m_pendingFrames.clear();
    m_totalFramesReceived = 0;
    endResetModel();
    
    CANFRAME_LOG_DEBUG("🗑 清空所有数据");
}

QVector<CANFrame> CANFrameTableModel::getAllFrames() const
{
    QVector<CANFrame> frames;
    frames.reserve(m_ringSize);
    
    for (int i = 0; i < m_ringSize; i++) {
        frames.append(getFrameAt(i));
    }
    
    return frames;
}

const CANFrame& CANFrameTableModel::getFrameAt(int index) const
{
    int realIndex = (m_ringTail + index) % m_ringCapacity;
    return m_ringBuffer[realIndex];
}

// ==================== 筛选控制 ====================

void CANFrameTableModel::setTypeFilter(int type)
{
    m_filterManager->setTypeFilter(type);
    applyFilters();
}

void CANFrameTableModel::setDirectionFilter(int dir)
{
    m_filterManager->setDirectionFilter(dir);
    applyFilters();
}

void CANFrameTableModel::setDataFilter(const QString &filter)
{
    m_filterManager->setDataFilter(filter);
    applyFilters();
}

void CANFrameTableModel::setIDFilter(const QString &id)
{
    m_filterManager->setIDFilter(id);
    applyFilters();
}

void CANFrameTableModel::setLengthFilter(const QString &length)
{
    m_filterManager->setLengthFilter(length);
    applyFilters();
}

void CANFrameTableModel::setChannelFilter(int channel)
{
    m_filterManager->setChannelFilter(channel);
    applyFilters();
}

void CANFrameTableModel::setSeqFilter(const QString &seq)
{
    m_filterManager->setSeqFilter(seq);
    applyFilters();
}

void CANFrameTableModel::setTimeFilter(const QString &time)
{
    m_filterManager->setTimeFilter(time);
    applyFilters();
}

void CANFrameTableModel::setIDFilterRule(const IDFilterRule &rule)
{
    m_filterManager->setIDFilterRule(rule);
    
    // 只有显示时过滤才需要重新应用筛选
    if (rule.mode == IDFilterRule::FilterOnDisplay) {
        applyFilters();
    }
}

IDFilterRule CANFrameTableModel::getIDFilterRule() const
{
    return m_filterManager->getIDFilterRule();
}

int CANFrameTableModel::getTypeFilter() const
{
    return m_filterManager->getTypeFilter();
}

int CANFrameTableModel::getDirectionFilter() const
{
    return m_filterManager->getDirectionFilter();
}

QString CANFrameTableModel::getDataFilter() const
{
    return m_filterManager->getDataFilter();
}

void CANFrameTableModel::applyFilters()
{
    // 先刷新待处理的帧
    if (!m_pendingFrames.isEmpty()) {
        flushPendingFrames();
    }
    
    beginResetModel();
    
    // 重新筛选所有数据
    m_filteredIndices.clear();
    // m_filteredIndices.reserve(m_ringSize); // std::deque 没有 reserve
    
    // 计算最早有效帧
    quint64 oldestValid = 0;
    if (m_totalFramesReceived > (quint64)m_ringCapacity) {
        oldestValid = m_totalFramesReceived - m_ringCapacity;
    }
    
    // 遍历当前环形缓冲区中的所有帧
    for (int i = 0; i < m_ringSize; i++) {
        // 计算绝对序号
        quint64 absIndex = oldestValid + i;
        
        const CANFrame &frame = getFrameAt(i);
        qint64 relativeTime = m_startTime.msecsTo(QDateTime::fromMSecsSinceEpoch(frame.timestamp / 1000)) * 1000;
        
        if (m_filterManager->shouldDisplayFrame(frame, absIndex + 1, relativeTime)) {
            m_filteredIndices.push_back(absIndex);
        }
    }
    
    endResetModel();
    
    CANFRAME_LOG_DEBUG("🔄 重新筛选: 从" << m_ringSize << "帧中筛选出" << m_filteredIndices.size() << "帧");
}

// ==================== 格式化方法 ====================

QString CANFrameTableModel::formatTime(quint64 timestamp) const
{
    if (m_timeFormat == 0) {
        // 🚀 阶段2优化：避免浮点运算，使用整数运算
        quint64 deltaUs = timestamp - m_startTimeUs;
        quint64 seconds = deltaUs / 1000000;
        quint64 micros = deltaUs % 1000000;
        
        // 预分配字符串（避免动态扩容）
        QString result;
        result.reserve(16);  // "123456.123456" 最多16字符
        
        result = QString::number(seconds);
        result += '.';
        
        // 微秒部分补零（6位）
        QString microsStr = QString::number(micros);
        int leadingZeros = 6 - microsStr.length();
        if (leadingZeros > 0) {
            result += QString(leadingZeros, '0');  // 前导零
        }
        result += microsStr;
        
        return result;
    } else {
        // 绝对时间（低频使用）才需要QDateTime
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp / 1000);
        return dt.toString("hh:mm:ss.zzz");
    }
}

QString CANFrameTableModel::formatID(quint32 id) const
{
    switch (m_idFormat) {
        case 0: {
            // 十六进制
            // ✅ 优化：使用查表法，避免QString::arg的格式化开销
            if (id <= 0x7FF) {
                // 标准帧 (11位): 0x000 - 0x7FF
                QString result = "0x";
                result += HEX_TABLE[(id >> 4) & 0xFF];
                result += HEX_TABLE[id & 0x0F][1];  // 只取第二个字符
                return result;
            } else {
                // 扩展帧 (29位): 使用传统方法（低频）
                return QString("0x%1").arg(id, 0, 16, QChar('0')).toUpper();
            }
        }
        
        case 1: {
            // 十进制
            return QString::number(id);
        }
        
        case 2: {
            // 二进制
            QString result;
            if (id <= 0x7FF) {
                // 标准帧 (11位)
                result.reserve(11);
                for (int bit = 10; bit >= 0; bit--) {
                    result += (id & (1 << bit)) ? '1' : '0';
                }
            } else {
                // 扩展帧 (29位)
                result.reserve(29);
                for (int bit = 28; bit >= 0; bit--) {
                    result += (id & (1 << bit)) ? '1' : '0';
                }
            }
            return result;
        }
        
        case 3: {
            // ASCII (对ID来说不适用，回退到十六进制)
            if (id <= 0x7FF) {
                QString result = "0x";
                result += HEX_TABLE[(id >> 4) & 0xFF];
                result += HEX_TABLE[id & 0x0F][1];
                return result;
            } else {
                return QString("0x%1").arg(id, 0, 16, QChar('0')).toUpper();
            }
        }
        
        default:
            return QString::number(id);
    }
}

QString CANFrameTableModel::formatData(const QByteArray &data) const
{
    switch (m_dataFormat) {
        case 0: {
            // 十六进制
            // ✅ 优化：使用查表法，避免toHex()的动态内存分配和格式化
            QString result;
            result.reserve(data.size() * 3);  // 预分配：每字节3字符 (XX + 空格)
            
            for (int i = 0; i < data.size(); i++) {
                if (i > 0) result += ' ';
                result += HEX_TABLE[(quint8)data[i]];  // 直接查表，O(1)
            }
            return result;
        }
        
        case 1: {
            // 十进制
            QString result;
            result.reserve(data.size() * 4);  // 预分配
            for (int i = 0; i < data.size(); i++) {
                if (i > 0) result += " ";
                result += QString::number((quint8)data[i]);
            }
            return result;
        }
        
        case 2: {
            // 二进制
            QString result;
            result.reserve(data.size() * 9);  // 预分配：每字节9字符 (8位 + 空格)
            for (int i = 0; i < data.size(); i++) {
                if (i > 0) result += ' ';
                quint8 byte = (quint8)data[i];
                // 转换为8位二进制字符串
                for (int bit = 7; bit >= 0; bit--) {
                    result += (byte & (1 << bit)) ? '1' : '0';
                }
            }
            return result;
        }
        
        case 3: {
            // ASCII
            QString result;
            result.reserve(data.size() * 2);  // 预分配
            for (int i = 0; i < data.size(); i++) {
                if (i > 0) result += ' ';
                quint8 byte = (quint8)data[i];
                // 可打印字符直接显示，否则显示'.'
                if (byte >= 32 && byte <= 126) {
                    result += QChar(byte);
                } else {
                    result += '.';
                }
            }
            return result;
        }
        
        default:
            return "";
    }
}

QString CANFrameTableModel::formatType(const CANFrame &frame) const
{
    if (frame.isCanFD) {
        return "CANFD";
    } else if (frame.isExtended) {
        return "扩展帧";
    } else {
        return "标准帧";
    }
}

QString CANFrameTableModel::formatDirection(bool isReceived) const
{
    return isReceived ? "Rx" : "Tx";
}
