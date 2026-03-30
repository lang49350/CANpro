/**
 * @file IDFilterMatcher.cpp
 * @brief 帧ID过滤匹配器实现
 * @author CANAnalyzerPro Team
 * @date 2024-02-09
 */

#include "IDFilterMatcher.h"
#include <QDebug>

bool IDFilterMatcher::matches(uint32_t frameID, const IDFilterRule &rule)
{
    // 如果过滤未启用，所有帧都匹配
    if (!rule.enabled) {
        return true;
    }
    
    // 根据模式选择匹配方式
    if (rule.isHexMode) {
        return matchesHexMode(frameID, rule);
    } else {
        return matchesStringMode(frameID, rule);
    }
}

bool IDFilterMatcher::matchesHexMode(uint32_t frameID, const IDFilterRule &rule)
{
    // 第一个条件匹配
    bool match1 = matchesCondition(frameID, rule.compareOp1, rule.value1, true);
    
    // 如果没有第二个条件，直接返回
    if (!rule.hasSecondCondition()) {
        return match1;
    }
    
    // 第二个条件匹配
    bool match2 = matchesCondition(frameID, rule.compareOp2, rule.value2, true);
    
    // 应用逻辑运算符
    if (rule.logicOp == IDFilterRule::And) {
        return match1 && match2;
    } else {
        return match1 || match2;
    }
}

bool IDFilterMatcher::matchesStringMode(uint32_t frameID, const IDFilterRule &rule)
{
    // 将帧ID转换为十六进制字符串（大写）
    QString idStr = QString::number(frameID, 16).toUpper();
    
    // 第一个条件匹配
    bool match1 = false;
    QString pattern1 = wildcardToRegex(rule.value1);
    QRegularExpression regex1(pattern1, QRegularExpression::CaseInsensitiveOption);
    
    if (rule.compareOp1 == IDFilterRule::Equal) {
        match1 = regex1.match(idStr).hasMatch();
    } else if (rule.compareOp1 == IDFilterRule::NotEqual) {
        match1 = !regex1.match(idStr).hasMatch();
    } else {
        qWarning() << "字符串模式不支持大于/小于比较";
        return false;
    }
    
    // 如果没有第二个条件，直接返回
    if (!rule.hasSecondCondition()) {
        return match1;
    }
    
    // 第二个条件匹配
    bool match2 = false;
    QString pattern2 = wildcardToRegex(rule.value2);
    QRegularExpression regex2(pattern2, QRegularExpression::CaseInsensitiveOption);
    
    if (rule.compareOp2 == IDFilterRule::Equal) {
        match2 = regex2.match(idStr).hasMatch();
    } else if (rule.compareOp2 == IDFilterRule::NotEqual) {
        match2 = !regex2.match(idStr).hasMatch();
    }
    
    // 应用逻辑运算符
    if (rule.logicOp == IDFilterRule::And) {
        return match1 && match2;
    } else {
        return match1 || match2;
    }
}

bool IDFilterMatcher::matchesCondition(uint32_t frameID, IDFilterRule::CompareOp op, const QString &value, bool /*isHexMode*/)
{
    if (value.trimmed().isEmpty()) {
        return true;  // 空值匹配所有
    }
    
    if (op == IDFilterRule::Equal) {
        // 等于：支持多个值，用逗号分隔（如：123, 345）
        QStringList values = value.split(',', Qt::SkipEmptyParts);
        for (const QString &val : values) {
            bool ok;
            uint32_t filterID = val.trimmed().toUInt(&ok, 16);
            if (ok && frameID == filterID) {
                return true;
            }
        }
        return false;
    } else if (op == IDFilterRule::NotEqual) {
        // 不等于：所有值都不匹配
        QStringList values = value.split(',', Qt::SkipEmptyParts);
        for (const QString &val : values) {
            bool ok;
            uint32_t filterID = val.trimmed().toUInt(&ok, 16);
            if (ok && frameID == filterID) {
                return false;
            }
        }
        return true;
    } else {
        // 大于/小于/大于等于/小于等于
        bool ok;
        uint32_t filterID = value.toUInt(&ok, 16);
        if (!ok) {
            qWarning() << "无效的十六进制值:" << value;
            return false;
        }
        
        switch (op) {
            case IDFilterRule::GreaterThan:
                return frameID > filterID;
            case IDFilterRule::LessThan:
                return frameID < filterID;
            case IDFilterRule::GreaterEqual:
                return frameID >= filterID;
            case IDFilterRule::LessEqual:
                return frameID <= filterID;
            default:
                return false;
        }
    }
}

QString IDFilterMatcher::wildcardToRegex(const QString &pattern)
{
    QString regex = pattern;
    
    // 转义正则表达式特殊字符（除了 ? 和 *）
    regex.replace("\\", "\\\\");
    regex.replace(".", "\\.");
    regex.replace("+", "\\+");
    regex.replace("^", "\\^");
    regex.replace("$", "\\$");
    regex.replace("(", "\\(");
    regex.replace(")", "\\)");
    regex.replace("[", "\\[");
    regex.replace("]", "\\]");
    regex.replace("{", "\\{");
    regex.replace("}", "\\}");
    regex.replace("|", "\\|");
    
    // 转换通配符
    regex.replace("?", ".");      // ? 匹配一个字符
    regex.replace("*", ".*");     // * 匹配任意字符
    
    // 添加开始和结束锚点
    return "^" + regex + "$";
}
