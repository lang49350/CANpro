/**
 * @file CANViewWidget.h
 * @brief CAN视图窗口 - 独立的CAN数据显示窗口
 * @author CANAnalyzerPro Team
 * @date 2024-02-08
 */

#ifndef CANVIEWWIDGET_H
#define CANVIEWWIDGET_H

#include <QWidget>
#include <QString>
#include <QTableView>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QElapsedTimer>  // 🚀 阶段1优化：自适应批量策略需要
#include <QScrollBar>
#include "../core/CANFrameTableModel.h"
#include "../core/CANFrame.h"
#include "../core/IDFilterRule.h"

// 前向声明
class CustomComboBox;
class CANViewRealTimeSaver;
class FilterHeaderWidget;
class HighlightDelegate;
class CANFrameDetailPanel;

/**
 * @brief CAN视图窗口（独立窗口）
 * 
 * 职责：
 * - 显示CAN数据
 * - 提供设备和通道选择
 * - 独立的数据模型和统计
 * - 数据筛选功能
 * 
 * 特点：
 * - 每个窗口完全独立
 * - 可以订阅不同设备的不同通道
 * - 独立的统计计数器
 */
class CANViewWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit CANViewWidget(QWidget *parent = nullptr);
    ~CANViewWidget();
    
    /**
     * @brief 设置要接收的设备
     * @param device 设备串口名称（如"COM3"）
     */
    void setDevice(const QString &device);
    
    /**
     * @brief 设置要接收的通道
     * @param channel 通道号（-1=ALL, 0=CAN0, 1=CAN1, ...）
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
     * @brief 获取接收帧计数
     */
    quint64 rxCount() const { return m_rxCount; }
    
    /**
     * @brief 获取发送帧计数
     */
    quint64 txCount() const { return m_txCount; }
    
    /**
     * @brief 获取错误帧计数
     */
    quint64 errorCount() const { return m_errorCount; }
    
    /**
     * @brief 更新设备列表（当有新设备连接时调用）
     */
    void updateDeviceList();
    
public slots:
    /**
     * @brief 接收单个CAN帧
     * @param frame CAN帧数据
     */
    void onFrameReceived(const CANFrame &frame);
    
    /**
     * @brief 批量接收CAN帧
     * @param frames CAN帧数据列表
     */
    void onFramesReceived(const QVector<CANFrame> &frames);
    
    /**
     * @brief 清空数据
     */
    void onClearClicked();
    
    /**
     * @brief 暂停/恢复显示
     */
    void onPauseClicked();
    
    /**
     * @brief 保存数据
     */
    void onSaveClicked();
    
    /**
     * @brief 实时保存按钮点击
     */
    void onRealTimeSaveClicked();
    
    /**
     * @brief 设置按钮点击
     */
    void onSettingsClicked();
    
    /**
     * @brief 切换详情面板显示/隐藏
     */
    void onToggleDetailPanel();
    
private:
    static constexpr int UI_FREEZE_THRESHOLD = 200000; // 与存储环容量对齐，避免冻结阈值高于可保留数据
    void setupUi();
    void setupConnections();
    void updateStatusBar();
    void loadSettings();        // 加载设置
    void applySettings();       // 应用设置（重新格式化显示）
    void applyColumnVisibility(const QMap<int, bool> &visibility);  // 应用列显示设置
    
    // 订阅管理
    void subscribe();
    void unsubscribe();
    
    // UI组件
    CustomComboBox *m_comboDevice;      // 设备选择器
    CustomComboBox *m_comboChannel;     // 通道选择器
    CustomComboBox *m_comboCanType;     // CAN 类型选择器（CAN2.0 / CANFD）
    QTableView *m_tableView;       // 表格视图
    FilterHeaderWidget *m_filterHeader;  // 筛选头部控件
    HighlightDelegate *m_highlightDelegate;  // 高亮显示委托
    CANFrameDetailPanel *m_detailPanel;  // 详情面板
    QPushButton *m_btnToggleDetail;  // 切换详情面板按钮
    QPushButton *m_btnPause;       // 暂停按钮
    QLabel *m_lblRxCount;          // 接收计数标签
    QLabel *m_lblTxCount;          // 发送计数标签
    QLabel *m_lblErrCount;         // 错误计数标签
    QLabel *m_lblFreezeHint;       // 冻结刷新提示
    QLabel *m_lblFilterStatus;     // 过滤状态标签
    
    // 核心对象
    CANFrameTableModel *m_tableModel;  // 表格数据模型（替代FrameModel）
    CANViewRealTimeSaver *m_realTimeSaver;  // 实时保存器
    
    // 状态
    QString m_currentDevice;       // 当前设备
    int m_currentChannel;          // 当前通道（-1=未选择）
    bool m_isPaused;               // 是否暂停
    
    // 独立统计
    quint64 m_rxCount;             // 接收帧计数
    quint64 m_txCount;             // 发送帧计数
    quint64 m_errorCount;          // 错误帧计数
    
    // 显示设置
    int m_timeFormat;              // 时间格式: 0=相对时间, 1=绝对时间
    int m_dataFormat;              // 数据格式: 0=十六进制, 1=十进制
    int m_idFormat;                // ID格式: 0=十六进制, 1=十进制
    QDateTime m_startTime;         // 开始时间（用于相对时间计算）
    QPushButton *m_btnRealTimeSave;  // 实时保存按钮
    
    // 帧ID过滤
    IDFilterRule m_idFilterRule;   // 帧ID过滤规则
    
    // 🚀 性能优化：UI刷新节流
    QVector<CANFrame> m_pendingFrames;  // 待处理的帧缓冲
    QTimer *m_uiUpdateTimer;             // UI更新定时器
    
    // 🚀 阶段1优化：自适应批量策略
    int m_adaptiveBatchSize;             // 自适应批量大小
    int m_adaptiveUIInterval;            // 自适应UI更新间隔
    QElapsedTimer m_frameRateTimer;      // 帧率计时器
    int m_frameCountInSecond;            // 每秒帧数计数
    bool m_exportInProgress;
    
    void updateAdaptiveStrategy();       // 更新自适应策略
    void refreshFrozenUiSnapshot();      // 在冻结模式下手动刷新最新数据
    void resetStatsBaseline();           // 重置统计基线（用于清空/切换订阅）
    
private slots:
    void onUIUpdateTimeout();  // UI更新定时器超时
    void onTableScrollChanged(int value); // 监听滚动到底部触发刷新

private:
    bool m_uiFrozenAfterCap = false;     // 达到阈值后冻结UI自动刷新
    bool m_frozenDataDirty = false;      // 冻结期间有新数据到达
    bool m_isApplyingFrozenSnapshot = false; // 防止刷新过程重入
    quint64 m_lastPulledSequence = 0;    // 显示层从存储层拉取的游标
    quint64 m_rxBase = 0;                // 接收计数基线（真实累计口径）
    quint64 m_txBase = 0;                // 发送计数基线（真实累计口径）
    quint64 m_errorBase = 0;             // 错误计数基线（真实累计口径）
};

#endif // CANVIEWWIDGET_H
