/**
 * @file FramePlaybackWidget.h
 * @brief 帧回放窗口 - 独立的CAN数据回放窗口
 * @author CANAnalyzerPro Team
 * @date 2026-02-25
 */

#ifndef FRAMEPLAYBACKWIDGET_H
#define FRAMEPLAYBACKWIDGET_H

#include <QWidget>
#include <QString>
#include <QTableView>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QTimer>
#include <QElapsedTimer>
#include "../core/CANFrame.h"
#include "../core/CANFrameTableModel.h"

// 前向声明
class CustomComboBox;
class FramePlaybackCore;

/**
 * @brief 帧回放窗口（独立窗口）
 * 
 * 职责：
 * - 提供回放文件的UI界面
 * - 显示加载的帧数据（类似CANViewWidget）
 * - 提供回放控制按钮
 * - 调用FramePlaybackCore执行回放
 * 
 * 特点：
 * - 支持多文件序列回放
 * - 支持倍速控制
 * - 支持步进回放
 * - 支持ID过滤
 */
class FramePlaybackWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit FramePlaybackWidget(QWidget *parent = nullptr);
    ~FramePlaybackWidget();
    
    /**
     * @brief 设置要使用的设备
     * @param device 设备串口名称（如"COM3"）
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
    
public slots:
    /**
     * @brief 加载文件到序列
     */
    void onLoadFile();
    
    /**
     * @brief 清空数据
     */
    void onClearData();
    
    /**
     * @brief 播放回放
     */
    void onPlay();
    
    /**
     * @brief 暂停回放
     */
    void onPause();
    
    /**
     * @brief 停止回放
     */
    void onStop();
    
    /**
     * @brief 单帧前进
     */
    void onStepForward();
    
    /**
     * @brief 单帧后退
     */
    void onStepBackward();
    
    /**
     * @brief 倍速改变
     */
    void onSpeedChanged(int index);
    
    /**
     * @brief 步进间隔改变
     */
    void onStepIntervalChanged(int value);
    
    /**
     * @brief 循环播放勾选改变
     */
    void onLoopChanged(int state);
    
    /**
     * @brief 使用原始时间戳勾选改变
     */
    void onUseOriginalTimingChanged(int state);
    
private:
    void setupUi();
    void setupConnections();
    void updateProgress();
    void updateButtonStates();
    void updateStatusBar();
    void updateDeviceList();  // 更新设备列表
    void updatePlaybackTimeLabel();
    
    // UI组件 - 工具栏
    CustomComboBox *m_comboDevice;      // 设备选择器
    CustomComboBox *m_comboChannel;     // 通道选择器（目标通道）
    CustomComboBox *m_comboSourceChannel; // 源通道选择器（过滤文件中的通道）
    CustomComboBox *m_comboCanType;     // CAN类型选择（CAN2.0 / CANFD）
    
    // UI组件 - 帧数据显示
    QTableView *m_tableView;            // 帧数据表格
    CANFrameTableModel *m_tableModel;   // 表格数据模型
    QPushButton *m_btnLoadFile;         // 加载文件按钮
    QPushButton *m_btnClearData;        // 清空数据按钮
    
    // UI组件 - 回放控制
    QPushButton *m_btnPlay;             // 播放按钮
    QPushButton *m_btnPause;            // 暂停按钮
    QPushButton *m_btnStop;             // 停止按钮
    QPushButton *m_btnStepBackward;     // 后退按钮
    QPushButton *m_btnStepForward;      // 前进按钮
    
    // UI组件 - 进度显示
    QProgressBar *m_progressBar;        // 进度条
    QLabel *m_lblProgress;              // 进度文本（帧数/总数）
    QLabel *m_lblTime;                  // 时间显示
    QLabel *m_lblFps;                   // 回放帧率显示
    QLabel *m_lblStatus;                // 状态显示
    
    // UI组件 - 回放设置
    QComboBox *m_comboSpeed;            // 倍速选择
    QSpinBox *m_spinStepInterval;       // 步进间隔
    QCheckBox *m_chkUseOriginalTiming;  // 使用原始时间戳
    QCheckBox *m_chkLoopSequence;       // 循环播放整个序列
    
    // 核心对象
    FramePlaybackCore *m_core;          // 回放核心逻辑
    
    // 状态
    QString m_currentDevice;            // 当前设备
    int m_currentChannel;               // 当前通道（0=CAN0, 1=CAN1）
    bool m_isPlaying;                   // 是否正在播放
    bool m_isPaused;                    // 是否暂停
    bool m_isSelectionPlayback;       // 右键“回放选中帧子集”模式，用于结束后释放通道申请
    QString m_currentFilename;          // 当前加载的文件名
    QTimer *m_elapsedUpdateTimer;       // 回放用时刷新定时器
    QElapsedTimer m_playbackElapsedTimer;
    qint64 m_accumulatedPausedMs;
    qint64 m_pauseStartedAtMs;
    int m_lastProgressFrame;            // 上次进度帧索引（用于帧率计算）
    qint64 m_lastProgressElapsedMs;     // 上次进度时间（ms）
    double m_smoothedPlaybackFps = -1.0; // 平滑后的回放帧率（EMA）
    
private slots:
    /**
     * @brief 回放进度更新
     * @param current 当前帧索引
     * @param total 总帧数
     */
    void onProgressUpdated(int current, int total);
    
    /**
     * @brief 回放完成
     */
    void onPlaybackFinished();
};

#endif // FRAMEPLAYBACKWIDGET_H
