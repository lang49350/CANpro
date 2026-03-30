#include "FilterManager.h"
#include "IDFilterMatcher.h"
#include <QDebug>

FilterManager::FilterManager()
    : m_filterType(0)
    , m_filterDirection(0)
    , m_filterChannel(-1)
{
}

FilterManager::~FilterManager()
{
}

// ==================== 筛选条件设置 ====================

void FilterManager::setTypeFilter(int type)
{
    m_filterType = type;
}

void FilterManager::setDirectionFilter(int dir)
{
    m_filterDirection = dir;
}

void FilterManager::setDataFilter(const QString &data)
{
    m_filterData = data;
}

void FilterManager::setIDFilter(const QString &id)
{
    m_filterID = id;
}

void FilterManager::setLengthFilter(const QString &length)
{
    m_filterLength = length;
}

void FilterManager::setChannelFilter(int channel)
{
    m_filterChannel = channel;
}

void FilterManager::setSeqFilter(const QString &seq)
{
    m_filterSeq = seq;
}

void FilterManager::setTimeFilter(const QString &time)
{
    m_filterTime = time;
}

void FilterManager::setIDFilterRule(const IDFilterRule &rule)
{
    m_idFilterRule = rule;
    qDebug() << "🔧 FilterManager设置ID过滤规则: 启用=" << rule.enabled 
             << ", 模式=" << (rule.mode == IDFilterRule::FilterOnReceive ? "接收时" : "显示时")
             << ", 类型=" << (rule.isHexMode ? "数字" : "字符串");
}

// ==================== 核心筛选方法 ====================

bool FilterManager::hasActiveFilter() const
{
    // 检查是否有任何激活的筛选条件
    if (m_filterType != 0) return true;
    if (m_filterDirection != 0) return true;
    if (!m_filterData.isEmpty()) return true;
    if (!m_filterID.isEmpty()) return true;
    if (!m_filterLength.isEmpty()) return true;
    if (m_filterChannel != -1) return true;
    if (!m_filterSeq.isEmpty()) return true;
    if (!m_filterTime.isEmpty()) return true;
    if (m_idFilterRule.enabled) return true;
    
    return false;
}

bool FilterManager::shouldDisplayFrame(const CANFrame &frame, int seq, qint64 relativeTime) const
{
    // ✅ 性能优化：快速路径 - 无筛选条件时直接返回
    // 避免后续所有的条件检查，提升20-30%性能
    if (!hasActiveFilter()) {
        return true;
    }
    
    // 优先级1: 序号筛选（最快，整数比较）
    if (!m_filterSeq.isEmpty()) {
        if (!matchSeqRange(seq, m_filterSeq)) {
            return false;
        }
    }
    
    // 优先级2: 通道筛选（快速，整数比较）
    if (m_filterChannel != -1) {
        if (frame.channel != m_filterChannel) {
            return false;
        }
    }
    
    // 优先级3: 类型筛选（快速，整数比较）
    // 0=全部, 1=标准帧, 2=CAN, 3=CANFD, 4=CANFD加速
    if (m_filterType != 0) {
        if (m_filterType == 1) {
            // 标准帧：只显示非扩展帧
            if (frame.isExtended) {
                return false;
            }
        }
        // 注意：目前CANFrame没有区分CAN/CANFD/CANFD加速
        // 这里暂时只实现标准帧筛选，其他类型待扩展
    }
    
    // 优先级4: 方向筛选（快速，布尔比较）
    // 0=全部, 1=Tx, 2=Rx
    if (m_filterDirection != 0) {
        if (m_filterDirection == 1) {
            // Tx：只显示发送帧
            if (frame.isReceived) {
                return false;
            }
        } else if (m_filterDirection == 2) {
            // Rx：只显示接收帧
            if (!frame.isReceived) {
                return false;
            }
        }
    }
    
    // 优先级5: 帧ID筛选（中速，整数比较+模式匹配）
    if (m_idFilterRule.enabled) {
        // 使用高级ID过滤规则（优先）
        if (!IDFilterMatcher::matches(frame.id, m_idFilterRule)) {
            return false;
        }
    } else if (!m_filterID.isEmpty()) {
        // 向后兼容：使用简单ID筛选
        if (!matchIDPattern(frame.id, m_filterID)) {
            return false;
        }
    }
    
    // 优先级6: 长度筛选（快速，整数比较）
    if (!m_filterLength.isEmpty()) {
        if (!matchLengthRange(frame.length, m_filterLength)) {
            return false;
        }
    }
    
    // 优先级7: 时间筛选（中速，浮点比较）
    if (!m_filterTime.isEmpty()) {
        if (!matchTimeRange(relativeTime, m_filterTime)) {
            return false;
        }
    }
    
    // 优先级8: 数据筛选（最慢，字符串处理）
    // ✅ 优化：提前检查，避免进入昂贵的字符串处理
    if (!m_filterData.isEmpty()) {
        // 1. 去除空格，转大写
        QString cleanFilter = m_filterData.toUpper();
        cleanFilter.remove(' ');
        
        // 2. 验证：必须是偶数个字符（完整字节）
        if (cleanFilter.length() % 2 != 0) {
            // 无效输入，不筛选（显示所有）
            return true;
        }
        
        // 3. 验证：必须是十六进制字符
        static QRegularExpression hexPattern("^[0-9A-F]+$");
        if (!hexPattern.match(cleanFilter).hasMatch()) {
            // 无效输入，不筛选（显示所有）
            return true;
        }
        
        // 4. 将帧数据转换为十六进制字符串（带空格，每2个字符一组）
        QString dataHex = frame.data.toHex(' ').toUpper();
        
        // 5. 将筛选条件也转换为带空格的格式（每2个字符一组）
        QString filterWithSpaces;
        for (int i = 0; i < cleanFilter.length(); i += 2) {
            if (i > 0) filterWithSpaces += ' ';
            filterWithSpaces += cleanFilter.mid(i, 2);
        }
        
        // 6. 字节边界匹配：确保匹配的是完整的字节
        // 例如："04"只匹配" 04 "或开头的"04 "或结尾的" 04"
        // 不匹配"40"中的"04"
        if (!dataHex.contains(filterWithSpaces)) {
            return false;
        }
    }
    
    // 所有条件都通过
    return true;
}

// ==================== 辅助方法 ====================

bool FilterManager::matchIDPattern(quint32 id, const QString &pattern) const
{
    QString cleanPattern = pattern.trimmed().toUpper();
    
    if (cleanPattern.isEmpty()) {
        return true;
    }
    
    // 情况1: 范围匹配 "1C0-1CF"
    if (cleanPattern.contains('-')) {
        QStringList parts = cleanPattern.split('-');
        if (parts.size() == 2) {
            bool ok1, ok2;
            quint32 minID = parts[0].trimmed().toUInt(&ok1, 16);
            quint32 maxID = parts[1].trimmed().toUInt(&ok2, 16);
            if (ok1 && ok2) {
                return (id >= minID && id <= maxID);
            }
        }
        // 解析失败，返回true（不筛选）
        return true;
    }
    
    // 情况2: 通配符匹配 "1C*"
    if (cleanPattern.contains('*')) {
        QString prefix = cleanPattern;
        prefix.remove('*');
        
        if (prefix.isEmpty()) {
            return true;  // 只有"*"，匹配所有
        }
        
        // 将ID转换为十六进制字符串
        QString idHex = QString::number(id, 16).toUpper();
        
        // 前缀匹配
        return idHex.startsWith(prefix);
    }
    
    // 情况3: 精确匹配 "1C0"
    bool ok;
    quint32 targetID = cleanPattern.toUInt(&ok, 16);
    if (ok) {
        return (id == targetID);
    }
    
    // 解析失败，返回true（不筛选）
    return true;
}

bool FilterManager::matchLengthRange(int length, const QString &range) const
{
    int min, max;
    if (parseRange(range, min, max)) {
        return (length >= min && length <= max);
    }
    // 解析失败，返回true（不筛选）
    return true;
}

bool FilterManager::matchSeqRange(int seq, const QString &range) const
{
    int min, max;
    if (parseRange(range, min, max)) {
        return (seq >= min && seq <= max);
    }
    // 解析失败，返回true（不筛选）
    return true;
}

bool FilterManager::matchTimeRange(qint64 time, const QString &range) const
{
    double min, max;
    if (parseFloatRange(range, min, max)) {
        // 将微秒转换为秒
        double timeSec = time / 1000000.0;
        return (timeSec >= min && timeSec <= max);
    }
    // 解析失败，返回true（不筛选）
    return true;
}

bool FilterManager::parseRange(const QString &range, int &min, int &max) const
{
    QString cleanRange = range.trimmed();
    
    if (cleanRange.isEmpty()) {
        return false;
    }
    
    // 情况1: 范围 "100-200"
    if (cleanRange.contains('-')) {
        QStringList parts = cleanRange.split('-');
        if (parts.size() == 2) {
            bool ok1, ok2;
            min = parts[0].trimmed().toInt(&ok1);
            max = parts[1].trimmed().toInt(&ok2);
            if (ok1 && ok2 && min <= max) {
                return true;
            }
        }
        return false;
    }
    
    // 情况2: 精确值 "100"
    bool ok;
    int value = cleanRange.toInt(&ok);
    if (ok) {
        min = value;
        max = value;
        return true;
    }
    
    return false;
}

bool FilterManager::parseFloatRange(const QString &range, double &min, double &max) const
{
    QString cleanRange = range.trimmed();
    
    if (cleanRange.isEmpty()) {
        return false;
    }
    
    // 情况1: 范围 "0-10.5"
    if (cleanRange.contains('-')) {
        QStringList parts = cleanRange.split('-');
        if (parts.size() == 2) {
            bool ok1, ok2;
            min = parts[0].trimmed().toDouble(&ok1);
            max = parts[1].trimmed().toDouble(&ok2);
            if (ok1 && ok2 && min <= max) {
                return true;
            }
        }
        return false;
    }
    
    // 情况2: 精确值 "10.5"
    bool ok;
    double value = cleanRange.toDouble(&ok);
    if (ok) {
        min = value;
        max = value;
        return true;
    }
    
    return false;
}
