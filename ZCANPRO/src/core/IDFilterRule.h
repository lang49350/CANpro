/**
 * @file IDFilterRule.h
 * @brief 帧ID过滤规则数据结构
 * @author CANAnalyzerPro Team
 * @date 2024-02-09
 */

#ifndef IDFILTERRULE_H
#define IDFILTERRULE_H

#include <QString>

/**
 * @brief 帧ID过滤规则
 * 
 * 支持两种模式：
 * 1. 数字(Hex)模式：十六进制数值比较
 * 2. 字符串模式：通配符匹配（? 代表一个字符，* 代表任意多个字符）
 * 
 * 支持两种过滤时机：
 * 1. 接收时过滤：节省内存，但数据丢失
 * 2. 显示时过滤：保留所有数据，可修改规则
 */
struct IDFilterRule
{
    /**
     * @brief 比较运算符
     */
    enum CompareOp {
        Equal,          // 等于
        NotEqual,       // 不等于
        GreaterThan,    // 大于
        LessThan,       // 小于
        GreaterEqual,   // 大于等于
        LessEqual       // 小于等于
    };
    
    /**
     * @brief 逻辑运算符
     */
    enum LogicOp {
        And,    // 与
        Or      // 或
    };
    
    /**
     * @brief 过滤模式
     */
    enum FilterMode {
        FilterOnReceive,   // 接收时过滤（节省内存，数据丢失）
        FilterOnDisplay    // 显示时过滤（保留数据，可修改规则）
    };
    
    bool enabled;           // 是否启用过滤
    FilterMode mode;        // 过滤模式
    bool isHexMode;         // true=数字(Hex), false=字符串
    CompareOp compareOp1;   // 第一个比较运算符
    QString value1;         // 第一个值
    LogicOp logicOp;        // 逻辑运算符（与/或）
    CompareOp compareOp2;   // 第二个比较运算符
    QString value2;         // 第二个值
    
    /**
     * @brief 默认构造函数
     */
    IDFilterRule()
        : enabled(false)
        , mode(FilterOnDisplay)  // 默认显示时过滤
        , isHexMode(true)
        , compareOp1(Equal)
        , logicOp(Or)
        , compareOp2(Equal)
    {
    }
    
    /**
     * @brief 检查是否有第二个条件
     */
    bool hasSecondCondition() const {
        return !value2.trimmed().isEmpty();
    }
};

#endif // IDFILTERRULE_H
