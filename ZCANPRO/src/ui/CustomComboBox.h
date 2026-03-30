/**
 * @file CustomComboBox.h
 * @brief 自定义下拉框，带有正确的三角形箭头
 * @author CANAnalyzerPro Team
 * @date 2024-02-09
 */

#ifndef CUSTOMCOMBOBOX_H
#define CUSTOMCOMBOBOX_H

#include <QComboBox>
#include <QPainter>
#include <QStyleOptionComboBox>
#include <QStylePainter>

/**
 * @brief 自定义下拉框类
 * 
 * 解决Qt QComboBox CSS三角形箭头显示问题
 * 通过重写paintEvent直接绘制三角形箭头
 */
class CustomComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit CustomComboBox(QWidget *parent = nullptr, bool orangeBackground = false);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    
private:
    bool m_orangeBackground;  // 是否使用橙色背景
};

#endif // CUSTOMCOMBOBOX_H
