/**
 * @file FramePlaybackWidget.cpp
 * @brief 帧回放窗口实现
 * @author CANAnalyzerPro Team
 * @date 2026-02-25
 */

#include "FramePlaybackWidget.h"
#include "CustomComboBox.h"
#include "../core/FramePlaybackCore.h"
#include "../core/FrameFileParser.h"
#include "../core/ConnectionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QHeaderView>
#include <QFileInfo>
#include "../core/LogManager.h"
#include <algorithm>

/**
 * @brief 构造函数
 */
FramePlaybackWidget::FramePlaybackWidget(QWidget *parent)
    : QWidget(parent)
    , m_comboDevice(nullptr)
    , m_comboChannel(nullptr)
    , m_comboSourceChannel(nullptr)
    , m_comboCanType(nullptr)
    , m_tableView(nullptr)
    , m_tableModel(nullptr)
    , m_btnLoadFile(nullptr)
    , m_btnClearData(nullptr)
    , m_btnPlay(nullptr)
    , m_btnPause(nullptr)
    , m_btnStop(nullptr)
    , m_btnStepBackward(nullptr)
    , m_btnStepForward(nullptr)
    , m_progressBar(nullptr)
    , m_lblProgress(nullptr)
    , m_lblTime(nullptr)
    , m_lblFps(nullptr)
    , m_lblStatus(nullptr)
    , m_comboSpeed(nullptr)
    , m_spinStepInterval(nullptr)
    , m_chkUseOriginalTiming(nullptr)
    , m_chkLoopSequence(nullptr)
    , m_core(nullptr)
    , m_currentDevice("")
    , m_currentChannel(0)
    , m_isPlaying(false)
    , m_isPaused(false)
    , m_isSelectionPlayback(false)
    , m_currentFilename("")
    , m_elapsedUpdateTimer(nullptr)
    , m_accumulatedPausedMs(0)
    , m_pauseStartedAtMs(-1)
    , m_lastProgressFrame(0)
    , m_lastProgressElapsedMs(0)
{
    // 创建核心逻辑对象
    m_core = new FramePlaybackCore(this);
    
    // 创建表格模型
    m_tableModel = new CANFrameTableModel(this);
    m_elapsedUpdateTimer = new QTimer(this);
    m_elapsedUpdateTimer->setInterval(200);
    
    setupUi();
    setupConnections();
    m_core->setStepInterval(m_spinStepInterval->value());
    updatePlaybackTimeLabel();
    updateButtonStates();
    updateDeviceList();  // 初始化设备列表
    LOG_DEBUG(LogCategory::UI, QStringLiteral("FramePlaybackWidget created"));
}

/**
 * @brief 鏋愭瀯鍑芥暟
 */
FramePlaybackWidget::~FramePlaybackWidget()
{
    LOG_DEBUG(LogCategory::UI, QStringLiteral("FramePlaybackWidget destroyed"));
    
    // 停止回放并释放通道模式申请
    if (m_core) {
        m_core->stop();
    }
    
    ConnectionManager *mgr = ConnectionManager::instance();
    if (!m_currentDevice.isEmpty()) {
        if (m_currentChannel == -1) {
            mgr->releaseChannelMode(m_currentDevice, 0, this);
            mgr->releaseChannelMode(m_currentDevice, 1, this);
        } else if (m_currentChannel >= 0) {
            mgr->releaseChannelMode(m_currentDevice, m_currentChannel, this);
        }
    }
}

/**
 * @brief 设置UI界面
 */
void FramePlaybackWidget::setupUi()
{
    // 涓诲竷灞€
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 1. Toolbar area
    QWidget *toolbarWidget = new QWidget(this);
    toolbarWidget->setFixedHeight(45);
    toolbarWidget->setStyleSheet("background-color: #E8EAF6;");
    
    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(10, 5, 10, 5);
    
    QLabel *lblDevice = new QLabel("设备:", toolbarWidget);
    m_comboDevice = new CustomComboBox(toolbarWidget);
    m_comboDevice->setMinimumWidth(120);
    
    QLabel *lblSourceChannel = new QLabel("源通道:", toolbarWidget);
    m_comboSourceChannel = new CustomComboBox(toolbarWidget);
    m_comboSourceChannel->addItem("全部通道", -1);
    m_comboSourceChannel->addItem("CAN0 (0)", 0);
    m_comboSourceChannel->addItem("CAN1 (1)", 1);
    m_comboSourceChannel->setMinimumWidth(100);

    QLabel *lblChannel = new QLabel("映射到通道:", toolbarWidget);
    m_comboChannel = new CustomComboBox(toolbarWidget);
    m_comboChannel->addItem("CAN0", 0);
    m_comboChannel->addItem("CAN1", 1);
    m_comboChannel->setMinimumWidth(100);

    QLabel *lblCanType = new QLabel("CAN类型:", toolbarWidget);
    m_comboCanType = new CustomComboBox(toolbarWidget);
    m_comboCanType->addItem("CAN2.0", false);
    m_comboCanType->addItem("CANFD", true);
    m_comboCanType->setMinimumWidth(100);
    m_comboCanType->setCurrentIndex(0);
    
    toolbarLayout->addWidget(lblDevice);
    toolbarLayout->addWidget(m_comboDevice);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(lblSourceChannel);
    toolbarLayout->addWidget(m_comboSourceChannel);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(lblChannel);
    toolbarLayout->addWidget(m_comboChannel);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(lblCanType);
    toolbarLayout->addWidget(m_comboCanType);
    toolbarLayout->addStretch();
    
    // 加载文件按钮
    m_btnLoadFile = new QPushButton("加载文件", toolbarWidget);
    m_btnLoadFile->setMinimumWidth(90);
    m_btnLoadFile->setMinimumHeight(30);
    m_btnLoadFile->setStyleSheet(
        "QPushButton { "
        "   background-color: #E3F2FD; "
        "   color: black; "
        "   border: 1px solid #90CAF9; "
        "   padding: 5px 15px; "
        "   font-size: 11px; "
        "   border-radius: 3px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #BBDEFB; "
        "   border: 1px solid #64B5F6; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #90CAF9; "
        "}"
    );
    toolbarLayout->addWidget(m_btnLoadFile);
    
    // 清空按钮
    m_btnClearData = new QPushButton("清空", toolbarWidget);
    m_btnClearData->setMinimumWidth(70);
    m_btnClearData->setMinimumHeight(30);
    m_btnClearData->setStyleSheet(
        "QPushButton { "
        "   background-color: #FFEBEE; "
        "   color: #C62828; "
        "   border: 1px solid #EF9A9A; "
        "   padding: 5px 15px; "
        "   font-size: 11px; "
        "   border-radius: 3px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #FFCDD2; "
        "   border: 1px solid #E57373; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #EF9A9A; "
        "}"
    );
    toolbarLayout->addWidget(m_btnClearData);
    
    mainLayout->addWidget(toolbarWidget);
    
    // 2. Main content area
    QWidget *contentWidget = new QWidget(this);
    QHBoxLayout *contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(10, 10, 10, 10);
    contentLayout->setSpacing(10);
    
    // 2.1 左侧：帧数据显示
    QVBoxLayout *leftLayout = new QVBoxLayout();
    
    // 文件信息标签
    m_lblStatus = new QLabel(QStringLiteral("未加载文件"), contentWidget);
    m_lblStatus->setStyleSheet("font-weight: bold; color: #666;");
    leftLayout->addWidget(m_lblStatus);
    
    // 表格视图
    m_tableView = new QTableView(contentWidget);
    m_tableView->setModel(m_tableModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    // Allow Ctrl/Shift range selection for "right-click send selected frames".
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setMinimumWidth(600);
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    leftLayout->addWidget(m_tableView);
    
    contentLayout->addLayout(leftLayout, 2);
    
    // 2.2 右侧：控制和设置
    QVBoxLayout *rightLayout = new QVBoxLayout();
    
    // 回放控制
    QGroupBox *controlGroup = new QGroupBox("回放控制", contentWidget);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
    
    // 控制按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_btnPlay = new QPushButton("▶ 播放", controlGroup);
    m_btnPause = new QPushButton("⏸ 暂停", controlGroup);
    m_btnStop = new QPushButton("⏹ 停止", controlGroup);
    
    buttonLayout->addWidget(m_btnPlay);
    buttonLayout->addWidget(m_btnPause);
    buttonLayout->addWidget(m_btnStop);
    controlLayout->addLayout(buttonLayout);
    
    // 步进按钮
    QHBoxLayout *stepLayout = new QHBoxLayout();
    m_btnStepBackward = new QPushButton("⏮ 后退", controlGroup);
    m_btnStepForward = new QPushButton("⏭ 前进", controlGroup);
    stepLayout->addWidget(m_btnStepBackward);
    stepLayout->addWidget(m_btnStepForward);
    controlLayout->addLayout(stepLayout);
    
    // Progress bar
    m_progressBar = new QProgressBar(controlGroup);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    controlLayout->addWidget(m_progressBar);
    
    // 进度信息
    QHBoxLayout *progressInfoLayout = new QHBoxLayout();
    m_lblProgress = new QLabel("0/0 帧", controlGroup);
    m_lblTime = new QLabel("用时: 00:00", controlGroup);
    progressInfoLayout->addWidget(m_lblProgress);
    progressInfoLayout->addStretch();
    progressInfoLayout->addWidget(m_lblTime);
    controlLayout->addLayout(progressInfoLayout);

    // 回放帧率显示
    m_lblFps = new QLabel("回放帧率: -- fps", controlGroup);
    m_lblFps->setStyleSheet("color: #455A64;");
    controlLayout->addWidget(m_lblFps);
    
    rightLayout->addWidget(controlGroup);
    
    // 回放设置
    QGroupBox *settingsGroup = new QGroupBox("回放设置", contentWidget);
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroup);
    
    // 倍速选择
    QHBoxLayout *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("倍速:", settingsGroup));
    m_comboSpeed = new QComboBox(settingsGroup);
    m_comboSpeed->addItem("0.5x", 0.5);
    m_comboSpeed->addItem("1.0x", 1.0);
    m_comboSpeed->addItem("2.0x", 2.0);
    m_comboSpeed->addItem("5.0x", 5.0);
    m_comboSpeed->setCurrentIndex(1);  // 默认1.0x
    speedLayout->addWidget(m_comboSpeed);
    speedLayout->addStretch();
    settingsLayout->addLayout(speedLayout);
    
    // 步进间隔
    QHBoxLayout *stepIntervalLayout = new QHBoxLayout();
    stepIntervalLayout->addWidget(new QLabel("步进间隔:", settingsGroup));
    m_spinStepInterval = new QSpinBox(settingsGroup);
    m_spinStepInterval->setRange(1, 1000);
    m_spinStepInterval->setValue(10);
    m_spinStepInterval->setSuffix(" ms");
    stepIntervalLayout->addWidget(m_spinStepInterval);
    stepIntervalLayout->addStretch();
    settingsLayout->addLayout(stepIntervalLayout);
    
    // 勾选项
    m_chkUseOriginalTiming = new QCheckBox("使用原始时间戳", settingsGroup);
    m_chkUseOriginalTiming->setChecked(true);
    settingsLayout->addWidget(m_chkUseOriginalTiming);
    
    m_chkLoopSequence = new QCheckBox("循环播放", settingsGroup);
    m_chkLoopSequence->setChecked(false);
    settingsLayout->addWidget(m_chkLoopSequence);
    
    settingsLayout->addStretch();
    
    rightLayout->addWidget(settingsGroup);
    rightLayout->addStretch();
    
    contentLayout->addLayout(rightLayout, 1);
    
    mainLayout->addWidget(contentWidget);
    
    setLayout(mainLayout);
}

/**
 * @brief 设置信号槽连接
 */
void FramePlaybackWidget::setupConnections()
{
    // 文件管理
    connect(m_btnLoadFile, &QPushButton::clicked, this, &FramePlaybackWidget::onLoadFile);
    connect(m_btnClearData, &QPushButton::clicked, this, &FramePlaybackWidget::onClearData);
    
    // 回放控制
    connect(m_btnPlay, &QPushButton::clicked, this, &FramePlaybackWidget::onPlay);
    connect(m_btnPause, &QPushButton::clicked, this, &FramePlaybackWidget::onPause);
    connect(m_btnStop, &QPushButton::clicked, this, &FramePlaybackWidget::onStop);
    connect(m_btnStepBackward, &QPushButton::clicked, this, &FramePlaybackWidget::onStepBackward);
    connect(m_btnStepForward, &QPushButton::clicked, this, &FramePlaybackWidget::onStepForward);
    
    // 设置变化
    connect(m_comboSpeed, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FramePlaybackWidget::onSpeedChanged);
    connect(m_spinStepInterval, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FramePlaybackWidget::onStepIntervalChanged);
    connect(m_chkLoopSequence, &QCheckBox::checkStateChanged,
            this, &FramePlaybackWidget::onLoopChanged);
    connect(m_chkUseOriginalTiming, &QCheckBox::checkStateChanged,
            this, &FramePlaybackWidget::onUseOriginalTimingChanged);
    
    // Device and channel selectors
    connect(m_comboDevice, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
        if (index >= 0) {
            QString device = m_comboDevice->itemData(index).toString();
            setDevice(device);
        }
    });
    
    connect(m_comboChannel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
        setChannel(index);
    });
    
    connect(m_comboSourceChannel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
        if (index >= 0) {
            int channel = m_comboSourceChannel->itemData(index).toInt();
            m_core->setSourceChannelFilter(channel);
        }
    });
    
    // 连接核心逻辑信号
    connect(m_core, &FramePlaybackCore::progressUpdated, 
            this, &FramePlaybackWidget::onProgressUpdated);
    connect(m_core, &FramePlaybackCore::playbackFinished, 
            this, &FramePlaybackWidget::onPlaybackFinished);
    connect(m_core, &FramePlaybackCore::errorOccurred, 
            this,
            [this](const QString &message) {
        QMessageBox::warning(this, "错误", message);
    });
    
    // Device connect/disconnect tracking
    connect(ConnectionManager::instance(), &ConnectionManager::deviceConnected,
            this, [this](const QString &) { updateDeviceList(); });
    connect(ConnectionManager::instance(), &ConnectionManager::deviceDisconnected,
            this, [this](const QString &port) {
        updateDeviceList();

        // If the currently selected device disconnects, immediately pause playback
        // to avoid continuing to enqueue/send frames.
        if (!m_currentDevice.isEmpty() && port == m_currentDevice) {
            if (m_isPlaying && !m_isPaused) {
                m_isPaused = true;
                m_elapsedUpdateTimer->stop();
                m_pauseStartedAtMs = m_playbackElapsedTimer.elapsed();
                updatePlaybackTimeLabel();
                m_core->pause();
                updateButtonStates();
            }
        }
    });

    // Right-click on the table to send selected frames once.
    connect(m_tableView, &QTableView::customContextMenuRequested, this,
            [this](const QPoint &pos) {
        if (!m_tableModel || !m_tableView) return;
        if (m_currentDevice.isEmpty()) return;

        const QModelIndex clickedIndex = m_tableView->indexAt(pos);
        if (!clickedIndex.isValid()) return;

        QItemSelectionModel *sel = m_tableView->selectionModel();
        if (!sel) return;

        const int clickedRow = clickedIndex.row();
        if (clickedRow < 0) return;

        // If user right-clicks an unselected row, select it (so "send selected" is intuitive).
        const QModelIndex clickedRowIndex = m_tableModel->index(clickedRow, 0);
        const bool clickedRowSelected = sel->isSelected(clickedRowIndex);
        if (!clickedRowSelected) {
            sel->clearSelection();
            sel->select(clickedRowIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

        QList<QModelIndex> selectedRows = sel->selectedRows(0);
        if (selectedRows.isEmpty()) {
            // Fallback: send clicked row only.
            selectedRows.append(clickedRowIndex);
        }

        // Sort selected rows by their row index so "sub playback" is deterministic.
        QList<int> rowNums;
        rowNums.reserve(selectedRows.size());
        for (const QModelIndex &rowIndex : selectedRows) {
            const int row = rowIndex.row();
            if (row >= 0) rowNums.push_back(row);
        }
        std::sort(rowNums.begin(), rowNums.end());

        QVector<CANFrame> rawFrames;
        rawFrames.reserve(rowNums.size());
        for (int row : rowNums) {
            if (row >= 0 && row < m_tableModel->rowCount()) {
                rawFrames.push_back(m_tableModel->getFrameAt(row));
            }
        }
        if (rawFrames.isEmpty()) return;

        // For "enqueueSendFrames" (instant send), channel must be the output channel explicitly.
        QVector<CANFrame> sendFrames = rawFrames;
        for (CANFrame &f : sendFrames) {
            f.channel = static_cast<quint8>(m_currentChannel);
            f.isReceived = false;
        }

        QMenu menu;
        QAction *actPlayback = menu.addAction(QStringLiteral("回放选中帧（子集）"));
        QAction *actSendOnce = menu.addAction(QStringLiteral("发送选中帧（一次）"));
        QAction *chosen = menu.exec(m_tableView->viewport()->mapToGlobal(pos));
        if (!chosen) return;

        const bool isFDWanted = (m_comboCanType ? m_comboCanType->currentData().toBool() : false);

        // 1) Right-click sub-playback (respects original timestamp / interval).
        if (chosen == actPlayback) {
            if (m_isPlaying) return;
            if (m_currentDevice.isEmpty()) return;

            ConnectionManager *mgr = ConnectionManager::instance();
            QString reason;

            auto requestTxOnly = [&]() -> bool {
                if (m_currentChannel == -1) {
                    if (!mgr->checkModeConflict(m_currentDevice, 0, ConnectionManager::MODE_TX_ONLY, reason) ||
                        !mgr->checkModeConflict(m_currentDevice, 1, ConnectionManager::MODE_TX_ONLY, reason)) {
                        return false;
                    }
                    mgr->requestChannelMode(m_currentDevice, 0, ConnectionManager::MODE_TX_ONLY, this, isFDWanted);
                    mgr->requestChannelMode(m_currentDevice, 1, ConnectionManager::MODE_TX_ONLY, this, isFDWanted);
                    return true;
                }

                bool conflict = !mgr->checkModeConflict(m_currentDevice, m_currentChannel,
                                                        ConnectionManager::MODE_TX_ONLY, reason);
                if (conflict) return false;
                mgr->requestChannelMode(m_currentDevice, m_currentChannel,
                                         ConnectionManager::MODE_TX_ONLY, this, isFDWanted);
                return true;
            };

            if (!requestTxOnly()) {
                QMessageBox::warning(this, QStringLiteral("模式冲突"), reason);
                return;
            }

            // Start "custom frames playback".
            if (m_core->playFrames(rawFrames)) {
                m_isSelectionPlayback = true;
                m_isPlaying = true;
                m_isPaused = false;

                m_accumulatedPausedMs = 0;
                m_pauseStartedAtMs = -1;
                m_lastProgressFrame = 0;
                m_lastProgressElapsedMs = 0;
                m_smoothedPlaybackFps = -1.0;

                m_playbackElapsedTimer.start();
                m_elapsedUpdateTimer->start();
                updatePlaybackTimeLabel();
                if (m_lblFps) {
                    m_lblFps->setText("回放帧率: -- fps");
                }
                updateButtonStates();
            } else {
                // Request was taken but playback failed.
                m_isSelectionPlayback = false;
                if (m_currentChannel == -1) {
                    mgr->releaseChannelMode(m_currentDevice, 0, this);
                    mgr->releaseChannelMode(m_currentDevice, 1, this);
                } else {
                    mgr->releaseChannelMode(m_currentDevice, m_currentChannel, this);
                }
            }
            return;
        }

        // 2) Instant send (one-shot), not timed.
        if (chosen == actSendOnce) {
            if (m_isPlaying) return;

            ConnectionManager *mgr = ConnectionManager::instance();
            QString reason;
            bool conflict = !mgr->checkModeConflict(m_currentDevice, m_currentChannel,
                                                     ConnectionManager::MODE_TX_ONLY, reason);
            if (conflict) {
                QMessageBox::warning(this, QStringLiteral("模式冲突"), reason);
                return;
            }

            mgr->requestChannelMode(m_currentDevice, m_currentChannel,
                                     ConnectionManager::MODE_TX_ONLY, this, isFDWanted);

            ConnectionManager::instance()->enqueueSendFrames(m_currentDevice, sendFrames);
            mgr->releaseChannelMode(m_currentDevice, m_currentChannel, this);
        }
    });
    connect(m_elapsedUpdateTimer, &QTimer::timeout, this, [this]() {
        updatePlaybackTimeLabel();
    });
}

/**
 * @brief 设置设备
 */
void FramePlaybackWidget::setDevice(const QString &device)
{
    m_currentDevice = device;
    m_core->setDevice(device);
    LOG_DEBUG(LogCategory::UI, QStringLiteral("FramePlaybackWidget set device: %1").arg(device));
}

/**
 * @brief 设置通道
 */
void FramePlaybackWidget::setChannel(int channel)
{
    m_currentChannel = channel;
    m_core->setChannel(channel);
    LOG_DEBUG(LogCategory::UI, QStringLiteral("FramePlaybackWidget set channel: %1").arg(channel));
}

/**
 * @brief 加载文件
 */
void FramePlaybackWidget::onLoadFile()
{
    QString filename = QFileDialog::getOpenFileName(
        this,
        "选择回放文件",
        "",
        "CAN日志文件 (*.blf *.asc *.csv);;所有文件 (*.*)"
    );
    
    if (filename.isEmpty()) {
        return;
    }
    
    LOG_DEBUG(LogCategory::UI, QStringLiteral("FramePlaybackWidget loading file: %1").arg(filename));
    
    // Parse file directly for preview and playback
    QVector<CANFrame> frames;
    QString errorMsg;
    
    if (!FrameFileParser::parseFile(filename, frames, errorMsg)) {
        QMessageBox::critical(this, "错误", "文件解析失败:\n" + errorMsg);
        return;
    }
    
    // 按时间戳排序
    std::sort(frames.begin(), frames.end(), [](const CANFrame &a, const CANFrame &b) {
        return a.timestamp < b.timestamp;
    });
    
    // 清空现有数据
    m_tableModel->clearFrames();
    
    // 添加帧数据到表格模型
    for (const CANFrame &frame : frames) {
        m_tableModel->addFrame(frame);
    }
    
    // 同时加载到核心逻辑（用于回放）
    if (m_core->loadFile(filename)) {
        m_currentFilename = filename;
        
        // 自动从文件中检测可用通道
        QSet<int> channels;
        for (const CANFrame &frame : frames) {
            channels.insert(frame.channel);
        }
        
        // Update source-channel combo box
        m_comboSourceChannel->blockSignals(true);
        m_comboSourceChannel->clear();
        m_comboSourceChannel->addItem("全部通道", -1);
        
        QList<int> sortedChannels = channels.values();
        std::sort(sortedChannels.begin(), sortedChannels.end());
        
        for (int ch : sortedChannels) {
            m_comboSourceChannel->addItem(QString("CAN%1 (%2)").arg(ch).arg(ch), ch);
        }
        m_comboSourceChannel->blockSignals(false);
        
        // Default to the detected channel when there is only one
        if (!sortedChannels.isEmpty()) {
            if (sortedChannels.size() == 1) {
                m_comboSourceChannel->setCurrentIndex(1);
            }
        }
        
        // 同步当前源通道过滤设置
        int sourceIdx = m_comboSourceChannel->currentIndex();
        if (sourceIdx >= 0) {
            int channel = m_comboSourceChannel->itemData(sourceIdx).toInt();
            m_core->setSourceChannelFilter(channel);
        }

        // Update status text
        QFileInfo fileInfo(filename);
        m_lblStatus->setText(QStringLiteral("已加载 %1（%2 帧）")
            .arg(fileInfo.fileName())
            .arg(frames.size()));
        
        // Update buttons
        updateButtonStates();
        
        LOG_DEBUG(LogCategory::UI, QStringLiteral("FramePlaybackWidget loaded %1 frames").arg(frames.size()));
    }
}

/**
 * @brief 清空数据
 */
void FramePlaybackWidget::onClearData()
{
    m_tableModel->clearFrames();
    m_core->clearSequences();
    m_currentFilename.clear();
    m_elapsedUpdateTimer->stop();
    m_accumulatedPausedMs = 0;
    m_pauseStartedAtMs = -1;
    m_lastProgressFrame = 0;
    m_lastProgressElapsedMs = 0;
    m_smoothedPlaybackFps = -1.0;
    updatePlaybackTimeLabel();
    if (m_lblFps) {
        m_lblFps->setText("回放帧率: -- fps");
    }
    m_lblStatus->setText(QStringLiteral("未加载文件"));
    updateButtonStates();
    
    LOG_DEBUG(LogCategory::UI, QStringLiteral("FramePlaybackWidget cleared data"));
}

/**
 * @brief 播放
 */
void FramePlaybackWidget::onPlay()
{
    // Mutual exclusion:
    // - When playback is running, right-click actions are blocked.
    // - When right-click subset playback is running, "Play" must not start again.
    if (m_isPlaying) {
        return;
    }
    
    // Normal playback (file sequence) keeps the channel mode request until Stop/finish.
    m_isSelectionPlayback = false;
    
    LOG_DEBUG(LogCategory::UI,
              QStringLiteral("FramePlaybackWidget::onPlay sequenceCount=%1 device=%2 channel=%3")
                  .arg(m_core->sequenceCount())
                  .arg(m_currentDevice)
                  .arg(m_currentChannel));
    
    if (m_core->sequenceCount() == 0) {
        QMessageBox::warning(this, "提示", "请先加载回放文件");
        return;
    }
    
    // 使用 ConnectionManager 请求 TX_ONLY 模式
    ConnectionManager *mgr = ConnectionManager::instance();
    const bool isFDWanted = (m_comboCanType ? m_comboCanType->currentData().toBool() : false);
    
    // Check mode conflict
    QString reason;
    bool conflict = false;
    if (m_currentChannel == -1) {
        if (!mgr->checkModeConflict(m_currentDevice, 0, ConnectionManager::MODE_TX_ONLY, reason) ||
            !mgr->checkModeConflict(m_currentDevice, 1, ConnectionManager::MODE_TX_ONLY, reason)) {
            conflict = true;
        }
    } else {
        if (!mgr->checkModeConflict(m_currentDevice, m_currentChannel, ConnectionManager::MODE_TX_ONLY, reason)) {
            conflict = true;
        }
    }

    if (conflict) {
        QMessageBox::warning(this, QStringLiteral("模式冲突"),
                             reason + QStringLiteral("\n请先关闭相关监听视图。"));
        return;
    }

    if (m_currentChannel == -1) {
        mgr->requestChannelMode(m_currentDevice, 0, ConnectionManager::MODE_TX_ONLY, this, isFDWanted);
        mgr->requestChannelMode(m_currentDevice, 1, ConnectionManager::MODE_TX_ONLY, this, isFDWanted);
    } else {
        mgr->requestChannelMode(m_currentDevice, m_currentChannel, ConnectionManager::MODE_TX_ONLY, this, isFDWanted);
    }
    
    if (m_core->play()) {
        m_accumulatedPausedMs = 0;
        m_pauseStartedAtMs = -1;
        m_lastProgressFrame = 0;
        m_lastProgressElapsedMs = 0;
        m_smoothedPlaybackFps = -1.0;
        m_playbackElapsedTimer.start();
        m_elapsedUpdateTimer->start();
        updatePlaybackTimeLabel();
        if (m_lblFps) {
            m_lblFps->setText("回放帧率: -- fps");
        }
        m_isPlaying = true;
        m_isPaused = false;
        updateButtonStates();
        LOG_DEBUG(LogCategory::UI, QStringLiteral("Frame playback started"));
    } else {
        m_isPlaying = false;
        m_isPaused = false;
        updateButtonStates();
    }
}

/**
 * @brief 暂停
 */
void FramePlaybackWidget::onPause()
{
    if (!m_isPlaying) {
        return;
    }

    m_isPaused = !m_isPaused;
    updateButtonStates();
    
    LOG_DEBUG(LogCategory::UI, QStringLiteral("Frame playback pause/resume toggled"));
    
    if (m_isPaused) {
        m_pauseStartedAtMs = m_playbackElapsedTimer.elapsed();
        m_elapsedUpdateTimer->stop();
        updatePlaybackTimeLabel();
        m_core->pause();
    } else {
        if (m_pauseStartedAtMs >= 0) {
            qint64 nowMs = m_playbackElapsedTimer.elapsed();
            m_accumulatedPausedMs += (nowMs - m_pauseStartedAtMs);
            m_pauseStartedAtMs = -1;
        }
        m_elapsedUpdateTimer->start();
        updatePlaybackTimeLabel();
        m_core->resume();
    }
}

/**
 * @brief 停止
 */
void FramePlaybackWidget::onStop()
{
    m_isSelectionPlayback = false;
    
    m_elapsedUpdateTimer->stop();
    m_accumulatedPausedMs = 0;
    m_pauseStartedAtMs = -1;
    m_lastProgressFrame = 0;
    m_lastProgressElapsedMs = 0;
    m_smoothedPlaybackFps = -1.0;
    updatePlaybackTimeLabel();
    if (m_lblFps) {
        m_lblFps->setText("回放帧率: -- fps");
    }
    m_isPlaying = false;
    m_isPaused = false;
    updateButtonStates();
    
    LOG_DEBUG(LogCategory::UI, QStringLiteral("Frame playback stopped"));
    
    // 释放通道模式申请
    ConnectionManager *mgr = ConnectionManager::instance();
    if (!m_currentDevice.isEmpty()) {
        if (m_currentChannel == -1) {
            mgr->releaseChannelMode(m_currentDevice, 0, this);
            mgr->releaseChannelMode(m_currentDevice, 1, this);
        } else if (m_currentChannel >= 0) {
            mgr->releaseChannelMode(m_currentDevice, m_currentChannel, this);
        }
    }
    
    // 调用核心逻辑停止
    m_core->stop();
}

/**
 * @brief 单帧前进
 */
void FramePlaybackWidget::onStepForward()
{
    LOG_DEBUG(LogCategory::UI, QStringLiteral("Frame playback step forward"));
    
    // 调用核心逻辑单帧前进
    m_core->stepForward();
}

/**
 * @brief 单帧后退
 */
void FramePlaybackWidget::onStepBackward()
{
    LOG_DEBUG(LogCategory::UI, QStringLiteral("Frame playback step backward"));
    
    // 调用核心逻辑单帧后退
    m_core->stepBackward();
}

/**
 * @brief 倍速改变
 */
void FramePlaybackWidget::onSpeedChanged(int index)
{
    double speed = m_comboSpeed->itemData(index).toDouble();
    LOG_DEBUG(LogCategory::UI, QStringLiteral("Frame playback speed changed: %1").arg(speed, 0, 'f', 2));

    // 调用核心逻辑设置倍速
    m_core->setPlaybackSpeed(speed);
}

/**
 * @brief 步进间隔改变
 */
void FramePlaybackWidget::onStepIntervalChanged(int value)
{
    LOG_DEBUG(LogCategory::UI, QStringLiteral("Frame playback step interval changed: %1 ms").arg(value));
    
    // 调用核心逻辑设置步进间隔
    m_core->setStepInterval(value);
}

/**
 * @brief 循环播放勾选改变
 */
void FramePlaybackWidget::onLoopChanged(int state)
{
    bool loop = (state == Qt::Checked);
    LOG_DEBUG(LogCategory::UI, QStringLiteral("Frame playback loop changed: %1").arg(loop));
    
    // 调用核心逻辑设置循环
    m_core->setLoopSequence(loop);
}

/**
 * @brief 使用原始时间戳勾选改变
 */
void FramePlaybackWidget::onUseOriginalTimingChanged(int state)
{
    bool useOriginal = (state == Qt::Checked);
    LOG_DEBUG(LogCategory::UI, QStringLiteral("Frame playback original timing changed: %1").arg(useOriginal));

    // 调用核心逻辑设置时间戳模式
    m_core->setUseOriginalTiming(useOriginal);
}

/**
 * @brief 更新进度
 */
void FramePlaybackWidget::updateProgress()
{
    // 从核心逻辑获取进度信息
    int current = m_core->currentFrameIndex();
    int total = m_core->totalFrames();
    
    if (total > 0) {
        int percent = (current * 100) / total;
        m_progressBar->setValue(percent);
        m_lblProgress->setText(QString("%1/%2 帧").arg(current).arg(total));
    } else {
        m_progressBar->setValue(0);
        m_lblProgress->setText("0/0 帧");
    }
}

/**
 * @brief 更新按钮状态
 */
void FramePlaybackWidget::updateButtonStates()
{
    bool hasData = (m_core->sequenceCount() > 0);
    
    m_btnPlay->setEnabled(hasData && !m_isPlaying);
    m_btnPause->setEnabled(m_isPlaying);
    m_btnStop->setEnabled(m_isPlaying);
    m_btnStepForward->setEnabled(hasData && !m_isPlaying);
    m_btnStepBackward->setEnabled(hasData && !m_isPlaying);
    m_btnClearData->setEnabled(hasData);

    // 避免回放过程中切换 CAN 类型导致模式/发送格式变化
    if (m_comboCanType) {
        m_comboCanType->setEnabled(!m_isPlaying);
    }
}

/**
 * @brief 更新状态栏
 */
void FramePlaybackWidget::updateStatusBar()
{
    // Update status text
    if (!m_currentFilename.isEmpty()) {
        QFileInfo fileInfo(m_currentFilename);
        m_lblStatus->setText(QStringLiteral("已加载 %1（%2 帧）")
            .arg(fileInfo.fileName())
            .arg(m_core->totalFrames()));
    } else {
        m_lblStatus->setText(QStringLiteral("未加载文件"));
    }
}

/**
 * @brief 回放进度更新槽函数
 */
void FramePlaybackWidget::onProgressUpdated(int current, int total)
{
    if (total > 0) {
        int percent = (current * 100) / total;
        m_progressBar->setValue(percent);
        m_lblProgress->setText(QString("%1/%2 帧").arg(current).arg(total));

        if (m_isPlaying && !m_isPaused && m_playbackElapsedTimer.isValid()) {
            qint64 elapsedMs = m_playbackElapsedTimer.elapsed() - m_accumulatedPausedMs;
            if (elapsedMs < 0) {
                elapsedMs = 0;
            }

            if (m_lastProgressElapsedMs > 0 && elapsedMs > m_lastProgressElapsedMs && current >= m_lastProgressFrame) {
                qint64 deltaMs = elapsedMs - m_lastProgressElapsedMs;
                int deltaFrames = current - m_lastProgressFrame;
                if (deltaMs > 0 && deltaFrames >= 0) {
                    const double fps = (static_cast<double>(deltaFrames) * 1000.0) / static_cast<double>(deltaMs);
                    constexpr double kAlpha = 0.25; // EMA平滑系数：越小越平稳
                    if (m_smoothedPlaybackFps < 0.0) {
                        m_smoothedPlaybackFps = fps;
                    } else {
                        m_smoothedPlaybackFps = (kAlpha * fps) + ((1.0 - kAlpha) * m_smoothedPlaybackFps);
                    }
                    if (m_lblFps) {
                        m_lblFps->setText(QString("回放帧率: %1 fps").arg(m_smoothedPlaybackFps, 0, 'f', 1));
                    }
                }
            }

            m_lastProgressFrame = current;
            m_lastProgressElapsedMs = elapsedMs;
        }
    }
}

/**
 * @brief 回放完成槽函数
 */
void FramePlaybackWidget::onPlaybackFinished()
{
    m_elapsedUpdateTimer->stop();
    m_pauseStartedAtMs = -1;
    m_lastProgressFrame = 0;
    m_lastProgressElapsedMs = 0;
    m_smoothedPlaybackFps = -1.0;
    updatePlaybackTimeLabel();
    if (m_lblFps) {
        m_lblFps->setText("回放帧率: -- fps");
    }
    m_isPlaying = false;
    m_isPaused = false;
    
    // If this playback was started from "right-click selected subset",
    // release the temporary TX_ONLY mode request now.
    if (m_isSelectionPlayback) {
        m_isSelectionPlayback = false;
        
        if (!m_currentDevice.isEmpty()) {
            if (m_currentChannel == -1) {
                ConnectionManager::instance()->releaseChannelMode(m_currentDevice, 0, this);
                ConnectionManager::instance()->releaseChannelMode(m_currentDevice, 1, this);
            } else {
                ConnectionManager::instance()->releaseChannelMode(m_currentDevice, m_currentChannel, this);
            }
        }
    }
    
    updateButtonStates();
    
    LOG_DEBUG(LogCategory::UI, QStringLiteral("Frame playback finished"));
}

void FramePlaybackWidget::updatePlaybackTimeLabel()
{
    qint64 elapsedMs = 0;
    if (m_isPlaying || m_isPaused) {
        elapsedMs = m_playbackElapsedTimer.isValid() ? m_playbackElapsedTimer.elapsed() : 0;
        elapsedMs -= m_accumulatedPausedMs;
        if (m_isPaused && m_pauseStartedAtMs >= 0) {
            elapsedMs -= (m_playbackElapsedTimer.elapsed() - m_pauseStartedAtMs);
        }
        if (elapsedMs < 0) {
            elapsedMs = 0;
        }
    }

    qint64 totalSeconds = elapsedMs / 1000;
    int minutes = static_cast<int>((totalSeconds / 60) % 60);
    int hours = static_cast<int>(totalSeconds / 3600);
    int seconds = static_cast<int>(totalSeconds % 60);

    if (hours > 0) {
        m_lblTime->setText(QString("用时: %1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0')));
    } else {
        m_lblTime->setText(QString("用时: %1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0')));
    }
}

/**
 * @brief 更新设备列表
 */
void FramePlaybackWidget::updateDeviceList()
{
    if (!m_comboDevice) return;
    
    // 保存当前选择
    QString currentDevice = m_currentDevice;
    
    // 清空列表
    m_comboDevice->clear();
    
    // 获取已连接设备列表
    QStringList devices = ConnectionManager::instance()->connectedDevices();
    
    if (devices.isEmpty()) {
        m_comboDevice->addItem(QStringLiteral("无可用设备"), QString());
        m_comboDevice->setEnabled(false);
    } else {
        m_comboDevice->setEnabled(true);
        for (const QString &device : devices) {
            m_comboDevice->addItem(device, device);
        }
        
        // 恢复之前的选择
        if (!currentDevice.isEmpty()) {
            int index = m_comboDevice->findData(currentDevice);
            if (index >= 0) {
                m_comboDevice->setCurrentIndex(index);
            }
        }
    }
    
    LOG_DEBUG(LogCategory::UI, QStringLiteral("FramePlaybackWidget refreshed device list, count=%1").arg(devices.size()));
}

