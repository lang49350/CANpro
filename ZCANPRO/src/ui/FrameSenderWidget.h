/**
 * @file FrameSenderWidget.h
 * @brief 帧发送窗口 - 周期发送和列表发送功能
 * @author CANAnalyzerPro Team
 * @date 2026-02-25
 */

#ifndef FRAMESENDERWIDGET_H
#define FRAMESENDERWIDGET_H

#include <QWidget>
#include <QString>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>

// 前向声明
class FrameSenderCore;
class CustomComboBox;

/**
 * @brief 帧发送窗口
 * 
 * 职责：
 * - 提供帧发送的UI界面
 * - 管理发送配置表格
 * - 调用FrameSenderCore执行发送
 * 
 * 特点：
 * - 支持多帧同时发送
 * - 支持ID/数据自增
 * - 支持配置保存/加载
 */
class FrameSenderWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit FrameSenderWidget(QWidget *parent = nullptr);
    ~FrameSenderWidget();
    
    /**
     * @brief 设置要使用的设备
     * @param device 设备串口名称（如"COM3"）
     * 
     * @note 设备必须已连接，否则发送会失败
     */
    void setDevice(const QString &device);
    
    /**
     * @brief 设置要使用的通道
     * @param channel 通道号（0=CAN0, 1=CAN1）
     */
    void setChannel(int channel);
    
    /**
     * @brief 获取当前设备
     * @return 设备串口名称
     */
    QString currentDevice() const { return m_currentDevice; }
    
    /**
     * @brief 获取当前通道
     * @return 通道号
     */
    int currentChannel() const { return m_currentChannel; }
    
    /**
     * @brief 加载配置文件
     * @param filename 配置文件路径
     */
    void loadConfig(const QString &filename);
    
    /**
     * @brief 保存配置文件
     * @param filename 配置文件路径
     */
    void saveConfig(const QString &filename);
    
public slots:
    /**
     * @brief 启动所有已启用的发送配置
     * 
     * @warning 如果设备未连接，会显示错误提示
     */
    void onStartAll();
    
    /**
     * @brief 停止所有发送
     */
    void onStopAll();
    
    /**
     * @brief 清空所有配置
     */
    void onClearAll();
    
    /**
     * @brief 加载配置按钮点击
     */
    void onLoadConfig();
    
    /**
     * @brief 保存配置按钮点击
     */
    void onSaveConfig();
    
    /**
     * @brief 添加新行
     */
    void onAddRow();
    
private slots:
    /**
     * @brief 表格单元格内容改变
     * @param row 行号
     * @param col 列号
     */
    void onCellChanged(int row, int col);
    
    /**
     * @brief 更新统计信息
     * @param totalSent 总发送帧数
     */
    void onStatisticsUpdated(int totalSent);
    
    /**
     * @brief 更新设备列表
     */
    void updateDeviceList();
    
private:
    void setupUi();
    void setupConnections();
    void updateStatusBar();
    
    // UI创建函数
    QWidget* createToolbar();
    QWidget* createTable();
    QWidget* createButtonBar();
    QWidget* createStatusBar();
    
    // UI组件
    CustomComboBox *m_comboDevice;      // 设备选择器
    CustomComboBox *m_comboChannel;     // 通道选择器
    CustomComboBox *m_comboCanType;     // CAN类型选择器（CAN2.0 / CANFD）
    QTableWidget *m_table;              // 配置表格
    QPushButton *m_btnStartAll;         // 启动全部按钮
    QPushButton *m_btnStopAll;          // 停止全部按钮
    QPushButton *m_btnClear;            // 清空按钮
    QPushButton *m_btnLoadConfig;       // 加载配置按钮
    QPushButton *m_btnSaveConfig;       // 保存配置按钮
    QPushButton *m_btnAddRow;           // 添加行按钮
    QLabel *m_lblStatus;                // 状态标签
    
    // 核心对象
    FrameSenderCore *m_core;            // 发送核心逻辑
    
    // 状态
    QString m_currentDevice;            // 当前设备
    int m_currentChannel;               // 当前通道
};

#endif // FRAMESENDERWIDGET_H
