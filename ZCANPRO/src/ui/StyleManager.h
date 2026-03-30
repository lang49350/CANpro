/**
 * @file StyleManager.h
 * @brief UI样式管理类
 * @author CANAnalyzerPro Team
 * @date 2024-02-10
 */

#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QString>

/**
 * @brief UI样式管理类
 * 
 * 统一管理所有UI组件的样式，避免重复代码
 */
class StyleManager
{
public:
    /**
     * @brief 获取标准ComboBox样式（白色背景）
     * @return 样式字符串
     */
    static QString comboBoxStyle();
    
    /**
     * @brief 获取橙色ComboBox样式（用于过滤器）
     * @return 样式字符串
     */
    static QString comboBoxOrangeStyle();
    
    /**
     * @brief 获取标准按钮样式
     * @return 样式字符串
     */
    static QString buttonStyle();
    
    /**
     * @brief 获取主按钮样式（蓝色）
     * @return 样式字符串
     */
    static QString primaryButtonStyle();
    
    /**
     * @brief 获取GroupBox样式
     * @return 样式字符串
     */
    static QString groupBoxStyle();
    
    /**
     * @brief 获取工具栏按钮样式
     * @return 样式字符串
     */
    static QString toolButtonStyle();
    
    /**
     * @brief 获取菜单样式
     * @return 样式字符串
     */
    static QString menuStyle();

private:
    StyleManager() = delete;  // 禁止实例化
};

#endif // STYLEMANAGER_H
