#ifndef FILTERMANAGER_H
#define FILTERMANAGER_H

#include <QString>
#include <QRegularExpression>
#include "CANFrame.h"
#include "IDFilterRule.h"

/**
 * @brief 筛选管理器
 * 
 * 负责管理所有CAN帧的筛选条件和筛选逻辑。
 * 支持多种筛选条件的组合（AND逻辑）。
 */
class FilterManager
{
public:
    FilterManager();
    ~FilterManager();
    
    // ==================== 筛选条件设置 ====================
    
    /**
     * @brief 设置类型筛选
     * @param type 0=全部, 1=标准帧, 2=CAN, 3=CANFD, 4=CANFD加速
     */
    void setTypeFilter(int type);
    
    /**
     * @brief 设置方向筛选
     * @param dir 0=全部, 1=Tx, 2=Rx
     */
    void setDirectionFilter(int dir);
    
    /**
     * @brief 设置数据筛选
     * @param data 十六进制字节，如"C6"或"C6 C7"
     */
    void setDataFilter(const QString &data);
    
    /**
     * @brief 设置帧ID筛选
     * @param id 支持通配符，如"1C*"或范围"1C0-1CF"或精确"1C0"
     */
    void setIDFilter(const QString &id);
    
    /**
     * @brief 设置长度筛选
     * @param length 支持精确值"8"或范围"4-8"
     */
    void setLengthFilter(const QString &length);
    
    /**
     * @brief 设置通道筛选
     * @param channel -1=全部, 0+=具体通道
     */
    void setChannelFilter(int channel);
    
    /**
     * @brief 设置序号筛选
     * @param seq 支持精确值"100"或范围"100-200"
     */
    void setSeqFilter(const QString &seq);
    
    /**
     * @brief 设置时间筛选
     * @param time 支持范围"0-10.5"（秒）
     */
    void setTimeFilter(const QString &time);
    
    /**
     * @brief 设置ID过滤规则（高级模式）
     * @param rule ID过滤规则
     */
    void setIDFilterRule(const IDFilterRule &rule);
    
    // ==================== 获取筛选条件 ====================
    
    int getTypeFilter() const { return m_filterType; }
    int getDirectionFilter() const { return m_filterDirection; }
    QString getDataFilter() const { return m_filterData; }
    QString getIDFilter() const { return m_filterID; }
    QString getLengthFilter() const { return m_filterLength; }
    int getChannelFilter() const { return m_filterChannel; }
    QString getSeqFilter() const { return m_filterSeq; }
    QString getTimeFilter() const { return m_filterTime; }
    IDFilterRule getIDFilterRule() const { return m_idFilterRule; }
    
    // ==================== 核心筛选方法 ====================
    
    /**
     * @brief 判断是否有激活的筛选条件
     * @return true=有筛选条件, false=无筛选条件
     */
    bool hasActiveFilter() const;
    
    /**
     * @brief 判断帧是否应该显示
     * @param frame CAN帧数据
     * @param seq 帧序号
     * @param relativeTime 相对时间（微秒）
     * @return true=显示, false=过滤掉
     */
    bool shouldDisplayFrame(const CANFrame &frame, int seq, qint64 relativeTime) const;
    
private:
    // ==================== 筛选条件 ====================
    
    int m_filterType;           // 类型筛选: 0=全部, 1=标准帧, 2=CAN, 3=CANFD, 4=CANFD加速
    int m_filterDirection;      // 方向筛选: 0=全部, 1=Tx, 2=Rx
    QString m_filterData;       // 数据筛选: 十六进制字节
    QString m_filterID;         // ID筛选: 支持通配符和范围
    QString m_filterLength;     // 长度筛选: 支持精确值和范围
    int m_filterChannel;        // 通道筛选: -1=全部, 0+=具体通道
    QString m_filterSeq;        // 序号筛选: 支持精确值和范围
    QString m_filterTime;       // 时间筛选: 支持范围（秒）
    IDFilterRule m_idFilterRule;  // ID过滤规则（高级）
    
    // ==================== 辅助方法 ====================
    
    /**
     * @brief 匹配ID模式
     * @param id 帧ID
     * @param pattern 模式字符串，如"1C*"或"1C0-1CF"或"1C0"
     * @return true=匹配, false=不匹配
     */
    bool matchIDPattern(quint32 id, const QString &pattern) const;
    
    /**
     * @brief 匹配长度范围
     * @param length 帧长度
     * @param range 范围字符串，如"8"或"4-8"
     * @return true=匹配, false=不匹配
     */
    bool matchLengthRange(int length, const QString &range) const;
    
    /**
     * @brief 匹配序号范围
     * @param seq 帧序号
     * @param range 范围字符串，如"100"或"100-200"
     * @return true=匹配, false=不匹配
     */
    bool matchSeqRange(int seq, const QString &range) const;
    
    /**
     * @brief 匹配时间范围
     * @param time 相对时间（微秒）
     * @param range 范围字符串，如"0-10.5"（秒）
     * @return true=匹配, false=不匹配
     */
    bool matchTimeRange(qint64 time, const QString &range) const;
    
    /**
     * @brief 解析范围字符串
     * @param range 范围字符串，如"100-200"
     * @param min 输出最小值
     * @param max 输出最大值
     * @return true=解析成功, false=解析失败
     */
    bool parseRange(const QString &range, int &min, int &max) const;
    
    /**
     * @brief 解析浮点范围字符串
     * @param range 范围字符串，如"0-10.5"
     * @param min 输出最小值
     * @param max 输出最大值
     * @return true=解析成功, false=解析失败
     */
    bool parseFloatRange(const QString &range, double &min, double &max) const;
};

#endif // FILTERMANAGER_H
