/**
 * @file StyleManager.cpp
 * @brief UI样式管理类实现
 * @author CANAnalyzerPro Team
 * @date 2024-02-10
 */

#include "StyleManager.h"

QString StyleManager::comboBoxStyle()
{
    return 
        "QComboBox { "
        "   border: 1px solid #CCCCCC; "
        "   padding: 3px 20px 3px 5px; "
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
        "}";
}

QString StyleManager::comboBoxOrangeStyle()
{
    return 
        "QComboBox { "
        "   background-color: #FFF3E0; "
        "   border: 1px solid #CCCCCC; "
        "   padding: 3px 20px 3px 5px; "
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
        "}";
}

QString StyleManager::buttonStyle()
{
    return 
        "QPushButton { "
        "   background-color: #E3F2FD; "
        "   color: black; "
        "   border: 1px solid #90CAF9; "
        "   padding: 5px 15px; "
        "   font-size: 12px; "
        "   border-radius: 3px; "
        "   min-width: 100px; "
        "   min-height: 35px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #BBDEFB; "
        "   border: 1px solid #64B5F6; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #90CAF9; "
        "}";
}

QString StyleManager::primaryButtonStyle()
{
    return 
        "QPushButton { "
        "   background-color: #2196F3; "
        "   color: white; "
        "   border: 1px solid #1976D2; "
        "   padding: 5px 15px; "
        "   font-size: 12px; "
        "   border-radius: 3px; "
        "   min-width: 100px; "
        "   min-height: 35px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #1E88E5; "
        "   border: 1px solid #1565C0; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #1976D2; "
        "}";
}

QString StyleManager::groupBoxStyle()
{
    return 
        "QGroupBox { "
        "   font-weight: bold; "
        "   border: 1px solid #CCCCCC; "
        "   border-radius: 5px; "
        "   margin-top: 10px; "
        "   padding-top: 10px; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   left: 10px; "
        "   padding: 0 5px; "
        "}";
}

QString StyleManager::toolButtonStyle()
{
    // 红色工具栏专用按钮样式
    return 
        "QToolButton { "
        "   background-color: rgba(0,0,0,0.15); "
        "   color: white; "
        "   border: 1px solid rgba(255,255,255,0.1); "
        "   padding-top: 10px; "
        "   padding-bottom: 6px; "
        "   padding-left: 12px; "
        "   padding-right: 12px; "
        "   font-size: 13px; "
        "   font-weight: normal; "
        "   font-family: 'Microsoft YaHei UI', 'Microsoft YaHei', sans-serif; "
        "   border-radius: 4px; "
        "   min-width: 75px; "
        "} "
        "QToolButton:hover { "
        "   background-color: rgba(255,255,255,0.25); "
        "   border: 1px solid rgba(255,255,255,0.5); "
        "   padding-top: 10px; "
        "   padding-bottom: 6px; "
        "   padding-left: 12px; "
        "   padding-right: 12px; "
        "} "
        "QToolButton:pressed { "
        "   background-color: rgba(255,255,255,0.35); "
        "   border: 1px solid rgba(255,255,255,0.6); "
        "   padding-top: 10px; "
        "   padding-bottom: 6px; "
        "   padding-left: 12px; "
        "   padding-right: 12px; "
        "} "
        "QToolButton::menu-indicator { "
        "   image: none; "
        "   width: 0px; "
        "}";
}

QString StyleManager::menuStyle()
{
    // 统一的菜单样式（白色背景）
    return 
        "QMenu { "
        "   background-color: white; "
        "   color: black; "
        "   border: 1px solid #CCCCCC; "
        "   padding: 5px; "
        "} "
        "QMenu::item { "
        "   padding: 8px 30px; "
        "   background-color: transparent; "
        "} "
        "QMenu::item:selected { "
        "   background-color: #E3F2FD; "
        "   color: black; "
        "} "
        "QMenu::separator { "
        "   height: 1px; "
        "   background-color: #E0E0E0; "
        "   margin: 5px 0px; "
        "}";
}
