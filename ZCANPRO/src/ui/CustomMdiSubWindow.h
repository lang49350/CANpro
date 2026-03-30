/**
 * @file CustomMdiSubWindow.h
 * @brief 自定义MDI子窗口 - 带自定义标题栏
 * @author CANAnalyzerPro Team
 * @date 2024-02-08
 */

#ifndef CUSTOMMDISUBWINDOW_H
#define CUSTOMMDISUBWINDOW_H

#include <QMdiSubWindow>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>

/**
 * @brief 自定义MDI子窗口
 * 
 * 功能：
 * - 自定义标题栏（蓝灰色背景）
 * - 支持拖动、调整大小
 * - 最小化、最大化、关闭按钮
 * - 最大化时填满MDI区域（不是系统最大化）
 */
class CustomMdiSubWindow : public QMdiSubWindow
{
    Q_OBJECT
    
public:
    explicit CustomMdiSubWindow(QWidget *parent = nullptr);
    ~CustomMdiSubWindow();
    
    /**
     * @brief 设置窗口内容widget
     */
    void setWidget(QWidget *widget);
    
    /**
     * @brief 设置窗口标题
     */
    void setWindowTitle(const QString &title);
    
    /**
     * @brief 检查窗口是否已最小化
     */
    bool isMinimized() const { return m_isMinimized; }
    
    /**
     * @brief 获取真正的内容widget（不是容器）
     */
    QWidget* contentWidget() const { return m_contentWidget; }
    
protected:
    // 鼠标事件处理（用于拖动标题栏和调整大小）
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    
private slots:
    void onMinimizeClicked();
    void onMaximizeClicked();
    void onCloseClicked();
    
private:
    void setupUi();
    QWidget* createTitleBar();
    
    // 边框检测
    enum ResizeEdge {
        None = 0,
        Top = 1,
        Bottom = 2,
        Left = 4,
        Right = 8,
        TopLeft = Top | Left,
        TopRight = Top | Right,
        BottomLeft = Bottom | Left,
        BottomRight = Bottom | Right
    };
    ResizeEdge getResizeEdge(const QPoint &pos);
    void updateCursor(ResizeEdge edge);
    
    // UI组件
    QWidget *m_titleBar;            // 自定义标题栏
    QLabel *m_lblTitle;             // 标题文字
    QPushButton *m_btnMinimize;     // 最小化按钮
    QPushButton *m_btnMaximize;     // 最大化按钮
    QPushButton *m_btnClose;        // 关闭按钮
    QWidget *m_contentWidget;       // 用户内容widget（用于最小化时隐藏）
    
    // 窗口拖动相关
    bool m_dragging;                // 是否正在拖动
    QPoint m_dragPosition;          // 拖动起始位置
    QPoint m_dragStartPos;          // 拖动起始鼠标位置（用于阈值检测）
    bool m_dragThresholdExceeded;   // 是否超过拖动阈值
    
    // 窗口调整大小相关
    bool m_resizing;                // 是否正在调整大小
    ResizeEdge m_resizeEdge;        // 调整大小的边缘
    QRect m_resizeStartGeometry;    // 调整大小前的几何位置
    QPoint m_resizeStartPos;        // 调整大小起始鼠标位置
    int m_borderWidth;              // 边框宽度（用于检测）
    
    // 最大化状态
    bool m_isMaximized;             // 是否已最大化
    QRect m_normalGeometry;         // 正常状态的几何位置
    
    // 最小化状态
    bool m_isMinimized;             // 是否已最小化
    QRect m_minimizedGeometry;      // 最小化后的几何位置
};

#endif // CUSTOMMDISUBWINDOW_H
