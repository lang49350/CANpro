/**
 * @file DeviceManagerDialog.h
 * @brief 设备管理对话框 - 管理所有CAN设备
 * @author CANAnalyzerPro Team
 * @date 2024-02-08
 */

#ifndef DEVICEMANAGERDIALOG_H
#define DEVICEMANAGERDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QTimer>

// 前向声明
class CustomComboBox;

/**
 * @brief 设备管理对话框
 * 
 * 功能：
 * - 显示所有可用串口设备
 * - 连接/断开设备
 * - 配置设备参数（波特率、通道数等）
 * - 显示设备状态和统计信息
 */
class DeviceManagerDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit DeviceManagerDialog(QWidget *parent = nullptr);
    ~DeviceManagerDialog();
    
protected:
    // 鼠标事件处理（用于窗口拖动）
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
private slots:
    void onRefreshClicked();        // 刷新设备列表
    void onConnectClicked();        // 连接选中设备
    void onDisconnectClicked();     // 断开选中设备
    void onConfigClicked();         // 配置选中设备
    void onDeviceSelectionChanged(); // 设备选择改变
    void updateDeviceList();        // 更新设备列表
    void updateDeviceStatus();      // 更新设备状态
    void onMaximizeClicked();       // 最大化按钮点击
    
private:
    void setupUi();
    void setupConnections();
    void addDeviceToTable(const QString &port, const QString &description, bool connected);
    int findDeviceRow(const QString &port);
    QWidget* createTitleBar();      // 创建自定义标题栏
    
    // UI组件
    QWidget *m_titleBar;            // 自定义标题栏
    QPushButton *m_btnMaximize;     // 最大化按钮
    QTableWidget *m_deviceTable;    // 设备列表表格
    QPushButton *m_btnRefresh;      // 刷新按钮
    QPushButton *m_btnConnect;      // 连接按钮
    QPushButton *m_btnDisconnect;   // 断开按钮
    QPushButton *m_btnConfig;       // 配置按钮
    QPushButton *m_btnClose;        // 关闭按钮
    
    // 配置区域
    CustomComboBox *m_comboBaudRate;     // 波特率选择
    QSpinBox *m_spinChannels;       // 通道数
    QLabel *m_lblStatus;            // 状态标签
    
    // 定时器
    QTimer *m_updateTimer;          // 状态更新定时器
    
    // 窗口拖动相关
    bool m_dragging;                // 是否正在拖动
    QPoint m_dragPosition;          // 拖动起始位置
    bool m_isMaximized;             // 是否已最大化
    QRect m_normalGeometry;         // 正常状态的几何位置
};

#endif // DEVICEMANAGERDIALOG_H
