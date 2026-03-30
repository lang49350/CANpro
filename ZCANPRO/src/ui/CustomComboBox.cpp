/**
 * @file CustomComboBox.cpp
 * @brief 自定义下拉框实现
 * @author CANAnalyzerPro Team
 * @date 2024-02-09
 */

#include "CustomComboBox.h"
#include <QStylePainter>
#include <QStyleOptionComboBox>
#include <QPainterPath>

CustomComboBox::CustomComboBox(QWidget *parent, bool orangeBackground)
    : QComboBox(parent)
    , m_orangeBackground(orangeBackground)
{
    // 设置基础样式
    if (m_orangeBackground) {
        // 橙色背景样式（用于过滤器）
        setStyleSheet(
            "QComboBox { "
            "   background-color: #FFF3E0; "
            "   border: 1px solid #CCCCCC; "
            "   padding: 3px 5px; "
            "   padding-right: 25px; "
            "} "
            "QComboBox:hover { "
            "   border: 1px solid #90CAF9; "
            "} "
            "QComboBox::drop-down { "
            "   subcontrol-origin: padding; "
            "   subcontrol-position: top right; "
            "   width: 20px; "
            "   border-left: 1px solid #CCCCCC; "
            "   background-color: transparent; "
            "} "
            "QComboBox::drop-down:hover { "
            "   background-color: #FFE0B2; "
            "} "
            "QComboBox::down-arrow { "
            "   image: url(:/down_arrow.png); "
            "} "
            "QComboBox::down-arrow:hover { "
            "   image: url(:/down_arrow_hover.png); "
            "} "
            "QComboBox::down-arrow:on { "
            "   image: url(:/down_arrow_pressed.png); "
            "}"
        );
    } else {
        // 标准白色背景样式
        setStyleSheet(
            "QComboBox { "
            "   border: 1px solid #CCCCCC; "
            "   padding: 3px 5px; "
            "   padding-right: 25px; "
            "   background-color: white; "
            "} "
            "QComboBox:hover { "
            "   border: 1px solid #90CAF9; "
            "} "
            "QComboBox::drop-down { "
            "   subcontrol-origin: padding; "
            "   subcontrol-position: top right; "
            "   width: 20px; "
            "   border-left: 1px solid #CCCCCC; "
            "   background-color: transparent; "
            "} "
            "QComboBox::drop-down:hover { "
            "   background-color: #E3F2FD; "
            "} "
            "QComboBox::down-arrow { "
            "   image: url(:/down_arrow.png); "
            "} "
            "QComboBox::down-arrow:hover { "
            "   image: url(:/down_arrow_hover.png); "
            "} "
            "QComboBox::down-arrow:on { "
            "   image: url(:/down_arrow_pressed.png); "
            "}"
        );
    }
}

void CustomComboBox::paintEvent(QPaintEvent *event)
{
    // 使用默认绘制，让全局样式表中的图片箭头生效
    QComboBox::paintEvent(event);
}
