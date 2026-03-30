#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QStatusBar>
#include <QMdiArea>
#include <QMdiSubWindow>

// 前向声明
class FramePlaybackWidget;

/**
 * @brief 主窗口类
 * 
 * CANAnalyzerPro的主界面 - MDI多文档界面
 * 
 * 职责：
 * - 管理MDI子窗口
 * - 提供工具栏和菜单
 * - 显示全局状态信息
 * - 自动排列子窗口
 * 
 * 架构说明：
 * - 使用ConnectionManager管理设备连接
 * - 使用DataRouter路由数据到各个窗口
 * - 使用CANViewWidget创建独立的CAN视图
 * - 使用CustomMdiSubWindow创建自定义标题栏的子窗口
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    // 获取MDI区域（供子对话框使用）
    QMdiArea* mdiArea() const { return m_mdiArea; }

protected:
    // 鼠标事件处理（用于窗口拖动和调整大小）
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;  // 用于处理HoverMove事件
    
    // 窗口大小改变事件（用于重新排列子窗口）
    void resizeEvent(QResizeEvent *event) override;
    
    // 窗口状态改变事件（用于更新最大化按钮图标）
    void changeEvent(QEvent *event) override;
    
private slots:
    // 设备管理
    void onConnectClicked();
    void onDisconnectClicked();
    
    // 视图管理
    void onNewCANView();
    void onNewDBCView();
    void onNewJ1939View();
    void onNewWaveformView();
    
    // 数据发送
    void onDataPlayback();
    void onFrameSender();
    
    // 性能监控
    void onShowStatistics();
    void onShowProtocolConfig();
    
    // 状态栏更新
    void updateStatusBar();
    
    // 窗口管理
    void onSubWindowClosed();
    
private:
    // UI初始化
    void setupUi();
    void setupConnections();
    void setupStatusBar();
    void setupToolbar();
    
    // 设置管理
    void loadSettings();
    void saveSettings();
    
    // 样式表加载
    void loadStyleSheet();              // 加载应用程序样式表
    
    // 辅助函数
    void updateAllWindowsDeviceList();  // 更新所有子窗口的设备列表
    void arrangeSubWindows();           // 自动排列子窗口
    
    // 边框检测和调整大小
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
    
    // UI控件
    QMdiArea *m_mdiArea;
    QStatusBar *statusBar;
    
    // 状态栏标签
    QLabel *m_statusLabel;
    QLabel *m_connectedDevicesLabel;  // 显示已连接设备
    QLabel *m_memoryLabel;
    QLabel *m_protocolModeLabel;      // 协议模式
    QLabel *m_throughputLabel;        // 吞吐量
    QLabel *m_latencyLabel;           // 延迟
    
    // 状态
    bool m_hasShownDisconnectError;  // 是否已显示断开连接错误
    
    // 更新定时器
    QTimer *m_updateTimer;
    QTimer *m_resizeTimer;  // 窗口大小改变延迟重排定时器
    
    // 窗口拖动相关
    bool m_dragging;
    QPoint m_dragPosition;
    
    // 窗口调整大小相关
    bool m_resizing;                // 是否正在调整大小
    ResizeEdge m_resizeEdge;        // 调整大小的边缘
    QRect m_resizeStartGeometry;    // 调整大小前的几何位置
    QPoint m_resizeStartPos;        // 调整大小起始鼠标位置
    int m_borderWidth;              // 边框宽度（用于检测）
    
    // 窗口控制按钮
    QPushButton *m_btnMaximize;     // 最大化按钮（需要更新图标）
    
    // 🔧 BUG修复：独立的窗口计数器（避免依赖异步销毁）
    int m_canWindowCount;           // CAN视图窗口数量（当前打开的窗口数）
    int m_dbcWindowCount;           // DBC视图窗口数量
    int m_j1939WindowCount;         // J1939视图窗口数量
    int m_waveformWindowCount;      // 波形视图窗口数量
    int m_playbackWindowCount;      // 回放窗口数量
    
    // 🔧 BUG修复：窗口编号计数器（永远递增，确保编号唯一）
    int m_canWindowNextId;          // 下一个CAN窗口的编号
    int m_dbcWindowNextId;          // 下一个DBC窗口的编号
    int m_j1939WindowNextId;        // 下一个J1939窗口的编号
    int m_waveformWindowNextId;     // 下一个波形窗口的编号
    int m_playbackWindowNextId;     // 下一个回放窗口的编号

    // 接收实时帧率计算（按相邻采样增量）
    quint32 m_lastRxFrames = 0;
    qint64 m_lastRxSampleMs = 0;
    QString m_lastRxDevice;
    double m_smoothedRxFps = -1.0;      // 平滑后的接收帧率（EMA）
};

#endif // MAINWINDOW_H
