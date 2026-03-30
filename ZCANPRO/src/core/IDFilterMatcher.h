/**
 * @file IDFilterMatcher.h
 * @brief 帧ID过滤匹配器
 * @author CANAnalyzerPro Team
 * @date 2024-02-09
 */

#ifndef IDFILTERMATCHER_H
#define IDFILTERMATCHER_H

#include "IDFilterRule.h"
#include <QRegularExpression>

/**
 * @brief 帧ID过滤匹配器
 * 
 * 负责根据过滤规则判断帧ID是否匹配
 */
class IDFilterMatcher
{
public:
    /**
     * @brief 检查帧ID是否匹配过滤规则
     * @param frameID 帧ID
     * @param rule 过滤规则
     * @return true=匹配（显示），false=不匹配（隐藏）
     */
    static bool matches(uint32_t frameID, const IDFilterRule &rule);
    
private:
    /**
     * @brief 数字(Hex)模式匹配
     */
    static bool matchesHexMode(uint32_t frameID, const IDFilterRule &rule);
    
    /**
     * @brief 字符串模式匹配（通配符）
     */
    static bool matchesStringMode(uint32_t frameID, const IDFilterRule &rule);
    
    /**
     * @brief 单个条件匹配
     */
    static bool matchesCondition(uint32_t frameID, IDFilterRule::CompareOp op, const QString &value, bool isHexMode);
    
    /**
     * @brief 通配符转正则表达式
     */
    static QString wildcardToRegex(const QString &pattern);
};

#endif // IDFILTERMATCHER_H
