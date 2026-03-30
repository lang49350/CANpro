#include "CANViewWidget.h"
#include "CANViewSettingsDialog.h"
#include "CANViewRealTimeSaver.h"
#include "CANFrameDetailPanel.h"
#include "CustomComboBox.h"
#include "FilterHeaderWidget.h"
#include "HighlightDelegate.h"
#include "../core/ConnectionManager.h"
#include "../core/DataRouter.h"
#include "../core/IDFilterMatcher.h"
#include "../core/SimpleXlsxWriter.h"
#include "../core/SerialConnection.h"
#include "../core/FrameStorageEngine.h"
#include "../core/zcan/ZCANStatistics.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QDebug>
#include <QRegularExpressionValidator>
#include <QSettings>
#include <QStandardItemModel>
#include <QApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QUrl>
#include <QHeaderView>
#include <QTimer>
#include <QElapsedTimer>  // 🚀 阶段1优化：自适应批量策略需要
#include <QSplitter>
#include <QThread>
#include <QPointer>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringConverter>

#ifndef CANVIEW_ADAPTIVE_STRATEGY_LOGGING
#define CANVIEW_ADAPTIVE_STRATEGY_LOGGING 0
#endif

namespace {
struct ExportTaskResult {
    bool ok;
    QString error;
    int frameCount;
    QString path;
};

ExportTaskResult exportFramesToFile(const QVector<CANFrame> &frames, const QString &fileName, const QString &selectedFilter)
{
    ExportTaskResult result{false, QString(), static_cast<int>(frames.size()), fileName};

    if (selectedFilter.contains("Excel") || selectedFilter.contains("xlsx")) {
        SimpleXlsxWriter writer;

        QStringList headers;
        headers << QString::fromUtf8("时间标签")
                << QString::fromUtf8("通道")
                << QString::fromUtf8("帧ID")
                << QString::fromUtf8("CAN类型")
                << QString::fromUtf8("方向")
                << QString::fromUtf8("长度")
                << QString::fromUtf8("数据");
        writer.addHeader(headers);

        for (const CANFrame &frame : frames) {
            QStringList row;
            double timestamp = frame.timestamp / 1000000.0;
            QString direction = frame.isReceived ? "Rx" : "Tx";
            QString idStr = QString::number(frame.id, 16).toUpper();
            QString canType = frame.isExtended ? QString::fromUtf8("扩展帧") : QString::fromUtf8("标准帧");
            QString dataStr = frame.data.toHex(' ').toUpper();

            row << QString::number(timestamp, 'f', 6)
                << QString::number(frame.channel + 1)
                << QString("0x%1").arg(idStr)
                << canType
                << direction
                << QString::number(frame.length)
                << dataStr;

            writer.addRow(row);
        }

        if (!writer.save(fileName)) {
            result.error = QString::fromUtf8("无法创建文件：") + fileName;
            return result;
        }

        result.ok = true;
        return result;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.error = QString::fromUtf8("无法创建文件：") + fileName;
        return result;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    if (selectedFilter.contains("CSV")) {
        out << QString::fromUtf8("时间标签,通道,帧ID,CAN类型,方向,长度,数据\n");

        for (const CANFrame &frame : frames) {
            double timestamp = frame.timestamp / 1000000.0;
            QString direction = frame.isReceived ? "Rx" : "Tx";
            QString idStr = QString::number(frame.id, 16).toUpper();
            QString canType = frame.isExtended ? QString::fromUtf8("扩展帧") : QString::fromUtf8("标准帧");
            QString dataStr = frame.data.toHex(' ').toUpper();

            out << QString("%1,%2,0x%3,%4,%5,%6,%7\n")
                .arg(timestamp, 0, 'f', 6)
                .arg(frame.channel + 1)
                .arg(idStr)
                .arg(canType)
                .arg(direction)
                .arg(frame.length)
                .arg(dataStr);
        }
    } else if (selectedFilter.contains("TXT")) {
        out << QString::fromUtf8("时间标签\t通道\t帧ID\tCAN类型\t方向\t长度\t数据\n");
        out << QString::fromUtf8("========================================================================================================\n");

        for (const CANFrame &frame : frames) {
            double timestamp = frame.timestamp / 1000000.0;
            QString direction = frame.isReceived ? "Rx" : "Tx";
            QString idStr = QString::number(frame.id, 16).toUpper();
            QString canType = frame.isExtended ? QString::fromUtf8("扩展帧") : QString::fromUtf8("标准帧");
            QString dataStr = frame.data.toHex(' ').toUpper();

            out << QString("%1\t%2\t0x%3\t%4\t%5\t%6\t%7\n")
                .arg(timestamp, 10, 'f', 6)
                .arg(frame.channel + 1)
                .arg(idStr.rightJustified(8))
                .arg(canType)
                .arg(direction)
                .arg(frame.length)
                .arg(dataStr);
        }
    } else {
        out << "date " << QDateTime::currentDateTime().toString("ddd MMM dd hh:mm:ss yyyy") << "\n";
        out << "base hex  timestamps absolute\n";
        out << "internal events logged\n";
        out << "Begin Triggerblock\n";

        for (const CANFrame &frame : frames) {
            double timestamp = frame.timestamp / 1000000.0;
            QString idStr = QString::number(frame.id, 16).toUpper();
            QString dataStr = frame.data.toHex(' ').toUpper();

            out << QString("   %1  %2  %3             d %4  %5\n")
                .arg(timestamp, 10, 'f', 6)
                .arg(frame.channel + 1)
                .arg(idStr.rightJustified(8))
                .arg(frame.length)
                .arg(dataStr);
        }

        out << "End TriggerBlock\n";
    }

    file.close();
    result.ok = true;
    return result;
}
}

CANViewWidget::CANViewWidget(QWidget *parent)
    : QWidget(parent)
    , m_tableModel(new CANFrameTableModel(this))
    , m_realTimeSaver(new CANViewRealTimeSaver(this))
    , m_currentDevice("")
    , m_currentChannel(-2)  // -2表示未选择，-1表示ALL，0+表示具体通道
    , m_isPaused(false)
    , m_rxCount(0)
    , m_txCount(0)
    , m_errorCount(0)
    , m_timeFormat(0)       // 默认相对时间
    , m_dataFormat(0)       // 默认十六进制
    , m_idFormat(0)         // 默认十六进制
    , m_startTime(QDateTime::currentDateTime())  // 记录开始时间
    , m_btnRealTimeSave(nullptr)
    , m_adaptiveBatchSize(300)      // 参数调优：提高初始批量大小
    , m_adaptiveUIInterval(250)     // 参数调优：降低UI刷新频率
    , m_frameCountInSecond(0)       // 🚀 阶段1优化：帧率计数器
    , m_exportInProgress(false)
{
    // 设置窗口最小尺寸，避免工具栏被过度压缩和详情面板内容重叠
    setMinimumWidth(900);
    setMinimumHeight(600);  // 设置最小高度，确保详情面板内容不重叠
    
    // 🚀 初始化UI更新定时器（性能优化：批量刷新）
    m_uiUpdateTimer = new QTimer(this);
    m_uiUpdateTimer->setInterval(250);  // 250ms = 4fps，降低高负载UI抖动
    connect(m_uiUpdateTimer, &QTimer::timeout, this, &CANViewWidget::onUIUpdateTimeout);
    m_pendingFrames.reserve(10000);  // 预分配空间
    
    // 🚀 阶段1优化：启动帧率计时器
    m_frameRateTimer.start();
    
    setupUi();
    setupConnections();
    
    // 加载设置（必须在setupUi之后，因为需要访问UI组件）
    loadSettings();
    
    updateDeviceList();
    
    qDebug() << "✅ CANViewWidget 创建";
}

CANViewWidget::~CANViewWidget()
{
    unsubscribe();
    
    // 停止实时保存
    if (m_realTimeSaver && m_realTimeSaver->isSaving()) {
        m_realTimeSaver->stopSaving();
    }
    
    
    qDebug() << "🗑 CANViewWidget 销毁";
}

void CANViewWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 1. 工具栏
    QWidget *toolbar = new QWidget();
    toolbar->setMinimumHeight(45);
    toolbar->setMaximumHeight(45);
    toolbar->setStyleSheet("QWidget { background-color: #E8EAF6; }");
    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(5, 5, 5, 5);
    toolbarLayout->setSpacing(3);
    
    // 设备选择
    QLabel *lblDevice = new QLabel("设备:");
    m_comboDevice = new CustomComboBox();
    m_comboDevice->addItem("请选择设备");
    m_comboDevice->setMinimumWidth(110);
    m_comboDevice->setMaximumWidth(180);
    
    // CAN 类型选择（须在通道之前；未选类型时通道不可选）
    QLabel *lblCanType = new QLabel("CAN类型:");
    m_comboCanType = new CustomComboBox();
    m_comboCanType->addItem(QString::fromUtf8("请选择CAN类型"));  // index=0
    m_comboCanType->addItem(QString::fromUtf8("CAN2.0"));         // index=1
    m_comboCanType->addItem(QString::fromUtf8("CANFD"));          // index=2
    m_comboCanType->setMinimumWidth(90);
    m_comboCanType->setMaximumWidth(120);
    m_comboCanType->setEnabled(false);

    // 通道选择
    QLabel *lblChannel = new QLabel("通道:");
    m_comboChannel = new CustomComboBox();
    m_comboChannel->addItem("请选择通道");
    m_comboChannel->addItem("ALL (所有通道)");
    m_comboChannel->addItem("CAN0 (通道0)");
    m_comboChannel->addItem("CAN1 (通道1)");
    m_comboChannel->setMinimumWidth(90);
    m_comboChannel->setMaximumWidth(150);
    m_comboChannel->setEnabled(false);

    // 禁用「请选择」占位项
    QStandardItemModel *canTypeModel = qobject_cast<QStandardItemModel*>(m_comboCanType->model());
    if (canTypeModel) {
        QStandardItem *ct0 = canTypeModel->item(0);
        if (ct0) {
            ct0->setFlags(ct0->flags() & ~Qt::ItemIsEnabled);
            ct0->setData(QColor(Qt::gray), Qt::ForegroundRole);
        }
    }
    QStandardItemModel *channelModel = qobject_cast<QStandardItemModel*>(m_comboChannel->model());
    if (channelModel) {
        QStandardItem *item0 = channelModel->item(0);
        if (item0) {
            item0->setFlags(item0->flags() & ~Qt::ItemIsEnabled);
            item0->setData(QColor(Qt::gray), Qt::ForegroundRole);
        }
    }
    
    // 工具按钮样式
    QString toolButtonStyle = 
        "QPushButton { "
        "   background-color: white; "
        "   border: 1px solid #CCCCCC; "
        "   padding: 5px 6px; "
        "   border-radius: 3px; "
        "   font-size: 11px; "
        "   min-width: 60px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #E3F2FD; "
        "   border: 1px solid #90CAF9; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #BBDEFB; "
        "}";
    
    // 工具按钮
    m_btnRealTimeSave = new QPushButton(QString::fromUtf8("实时保存"));
    m_btnRealTimeSave->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_btnRealTimeSave->setStyleSheet(toolButtonStyle);
    m_btnRealTimeSave->setToolTip(QString::fromUtf8("实时保存到文件"));
    m_btnRealTimeSave->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    QPushButton *btnSave = new QPushButton(QString::fromUtf8("保存"));
    btnSave->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    btnSave->setStyleSheet(toolButtonStyle);
    btnSave->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    QPushButton *btnClear = new QPushButton(QString::fromUtf8("清空"));
    btnClear->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    btnClear->setStyleSheet(toolButtonStyle);
    btnClear->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    m_btnPause = new QPushButton(QString::fromUtf8("暂停"));
    m_btnPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    m_btnPause->setStyleSheet(toolButtonStyle);
    m_btnPause->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    QPushButton *btnSettings = new QPushButton(QString::fromUtf8("设置"));
    btnSettings->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    btnSettings->setStyleSheet(toolButtonStyle);
    btnSettings->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    // 过滤状态标签
    m_lblFilterStatus = new QLabel(QString::fromUtf8("  过滤已启用"));
    m_lblFilterStatus->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 11px; }");
    m_lblFilterStatus->setVisible(false);  // 默认隐藏
    m_lblFilterStatus->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    toolbarLayout->addWidget(lblDevice);
    toolbarLayout->addWidget(m_comboDevice);
    toolbarLayout->addSpacing(5);
    toolbarLayout->addWidget(lblCanType);
    toolbarLayout->addWidget(m_comboCanType);
    toolbarLayout->addSpacing(5);
    toolbarLayout->addWidget(lblChannel);
    toolbarLayout->addWidget(m_comboChannel);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(m_btnRealTimeSave);
    toolbarLayout->addWidget(btnSave);
    toolbarLayout->addWidget(btnClear);
    toolbarLayout->addWidget(m_btnPause);
    toolbarLayout->addWidget(btnSettings);
    toolbarLayout->addWidget(m_lblFilterStatus);
    toolbarLayout->addStretch();  // 添加弹性空间，让按钮靠左对齐
    mainLayout->addWidget(toolbar);
    
    // 连接工具栏按钮
    connect(btnSave, &QPushButton::clicked, this, &CANViewWidget::onSaveClicked);
    connect(btnClear, &QPushButton::clicked, this, &CANViewWidget::onClearClicked);
    connect(m_btnRealTimeSave, &QPushButton::clicked, this, &CANViewWidget::onRealTimeSaveClicked);
    connect(btnSettings, &QPushButton::clicked, this, &CANViewWidget::onSettingsClicked);
    
    // 2. 数据显示区（QTableView）
    m_tableView = new QTableView();
    m_tableView->setModel(m_tableModel);
    m_tableView->setStyleSheet(
        "QTableView { "
        "   background-color: white; "
        "   color: black; "
        "   font-family: 'Consolas', 'Courier New', monospace; "
        "   font-size: 10pt; "
        "   selection-background-color: #BBDEFB; "
        "   gridline-color: #E0E0E0; "
        "}"
        "QHeaderView::section { "
        "   background-color: #F5F5F5; "
        "   padding: 5px; "
        "   border: 1px solid #D0D0D0; "
        "   font-weight: bold; "
        "}"
    );
    
    // 设置表格属性
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setShowGrid(true);
    m_tableView->verticalHeader()->setVisible(false);  // 隐藏行号
    m_tableView->horizontalHeader()->setStretchLastSection(true);  // 最后一列自动拉伸
    
    // 设置列宽（新顺序：序号、时间、通道、ID、类型、方向、长度、数据）
    m_tableView->setColumnWidth(CANFrameTableModel::COL_SEQ, 60);
    m_tableView->setColumnWidth(CANFrameTableModel::COL_TIME, 120);
    m_tableView->setColumnWidth(CANFrameTableModel::COL_CHANNEL, 50);
    m_tableView->setColumnWidth(CANFrameTableModel::COL_ID, 100);
    m_tableView->setColumnWidth(CANFrameTableModel::COL_TYPE, 80);
    m_tableView->setColumnWidth(CANFrameTableModel::COL_DIR, 80);
    m_tableView->setColumnWidth(CANFrameTableModel::COL_LEN, 60);
    m_tableView->setColumnWidth(CANFrameTableModel::COL_DATA, 350);  // 数据列变长
    
    // 3. 筛选头部控件（放在表格上方，紧贴表头）
    m_filterHeader = new FilterHeaderWidget(m_tableView, this);
    m_filterHeader->updateColumnWidths();
    
    // 4. 创建并设置高亮委托
    m_highlightDelegate = new HighlightDelegate(this);
    m_tableView->setItemDelegate(m_highlightDelegate);
    
    // 5. 创建详情面板
    m_detailPanel = new CANFrameDetailPanel(this);
    m_detailPanel->setMinimumWidth(300);
    m_detailPanel->setMaximumWidth(400);
    
    // 5.1 创建切换按钮（小箭头，独立放置）
    m_btnToggleDetail = new QPushButton(QString::fromUtf8("▶"));  // 默认显示时用▶
    m_btnToggleDetail->setFixedSize(12, 60);  // 更窄：12×60
    m_btnToggleDetail->setToolTip(QString::fromUtf8("隐藏详情面板"));
    m_btnToggleDetail->setStyleSheet(
        "QPushButton { "
        "   background-color: #E8EAF6; "
        "   border: 1px solid #CCCCCC; "
        "   border-radius: 3px; "
        "   font-size: 10px; "
        "   padding: 0px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #C5CAE9; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #9FA8DA; "
        "}"
    );
    
    // 6. 使用分割器组合表格和详情面板
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    
    // 左侧：表格容器
    QWidget *tableContainer = new QWidget();
    QVBoxLayout *tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);
    tableLayout->addWidget(m_filterHeader);
    tableLayout->addWidget(m_tableView);
    
    // 中间：按钮容器（独立，始终可见）
    QWidget *btnContainer = new QWidget();
    QVBoxLayout *btnLayout = new QVBoxLayout(btnContainer);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnToggleDetail);
    btnLayout->addStretch();
    btnContainer->setMaximumWidth(16);  // 按钮容器更窄
    
    splitter->addWidget(tableContainer);
    splitter->addWidget(btnContainer);
    splitter->addWidget(m_detailPanel);
    
    // 设置分割比例
    splitter->setStretchFactor(0, 7);   // 表格
    splitter->setStretchFactor(1, 0);   // 按钮（不拉伸）
    splitter->setStretchFactor(2, 3);   // 详情面板
    
    // 禁止调整按钮容器的大小
    splitter->handle(1)->setEnabled(false);
    
    mainLayout->addWidget(splitter);
    
    // 7. 底部状态栏
    QWidget *statusBar = new QWidget();
    statusBar->setMinimumHeight(30);
    statusBar->setMaximumHeight(30);
    statusBar->setStyleSheet("QWidget { background-color: #ECEFF1; }");
    QHBoxLayout *statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(10, 0, 10, 0);
    
    QCheckBox *chkShowError = new QCheckBox("显示错误信息");
    m_lblRxCount = new QLabel("接收帧计数: 0");
    m_lblTxCount = new QLabel("发送帧计数: 0");
    m_lblErrCount = new QLabel("错误帧计数: 0");
    m_lblFreezeHint = new QLabel("实时刷新中");
    m_lblFreezeHint->setStyleSheet("QLabel { color: #455A64; }");
    
    statusLayout->addWidget(chkShowError);
    statusLayout->addSpacing(12);
    statusLayout->addWidget(m_lblFreezeHint);
    statusLayout->addStretch();
    statusLayout->addWidget(m_lblRxCount);
    statusLayout->addSpacing(20);
    statusLayout->addWidget(m_lblTxCount);
    statusLayout->addSpacing(20);
    statusLayout->addWidget(m_lblErrCount);
    
    mainLayout->addWidget(statusBar);
}

void CANViewWidget::setupConnections()
{
    // 设备选择信号
    connect(m_comboDevice, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (index > 0) {
            QString deviceText = m_comboDevice->currentText();
            QString device = deviceText.split(" - ").first();
            setDevice(device);
            m_comboCanType->setEnabled(true);
            m_comboCanType->setCurrentIndex(0);  // 请先选 CAN 类型
            m_comboChannel->setEnabled(false);
            m_comboChannel->setCurrentIndex(0);
        } else {
            DataRouter *router = DataRouter::instance();
            router->unsubscribe(this);
            disconnect(router, &DataRouter::framesRoutedToWidget, this, nullptr);
            
            m_currentDevice = "";
            m_currentChannel = -2;
            m_comboCanType->setEnabled(false);
            m_comboCanType->setCurrentIndex(0);
            m_comboChannel->setEnabled(false);
            m_comboChannel->setCurrentIndex(0);
        }
    });
    
    // 通道选择信号（须已选设备且已选 CAN 类型）
    connect(m_comboChannel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (index > 0 && !m_currentDevice.isEmpty() && m_comboCanType->currentIndex() > 0) {
            int channel;
            if (index == 1) {
                channel = -1;  // ALL
            } else {
                channel = index - 2;  // CAN0=0, CAN1=1
            }
            setChannel(channel);
        } else {
            if (!m_currentDevice.isEmpty()) {
                unsubscribe();
                m_currentChannel = -2;
                qDebug() << "🔕 取消订阅（未选择通道）";
            }
        }
    });

    // CAN 类型：index 0=未选；1=CAN2.0；2=CANFD。未选类型时禁用通道。
    connect(m_comboCanType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (m_currentDevice.isEmpty()) {
            return;
        }

        if (index <= 0) {
            m_comboChannel->setEnabled(false);
            m_comboChannel->setCurrentIndex(0);
            if (m_currentChannel >= -1) {
                unsubscribe();
            }
            return;
        }

        m_comboChannel->setEnabled(true);

        const bool isFDWanted = (index == 2);

        if (m_currentChannel == -1) {
            ConnectionManager *mgr = ConnectionManager::instance();
            mgr->requestChannelMode(m_currentDevice, 0, ConnectionManager::MODE_RX_ONLY, this, isFDWanted);
            mgr->requestChannelMode(m_currentDevice, 1, ConnectionManager::MODE_RX_ONLY, this, isFDWanted);
        } else if (m_currentChannel >= 0) {
            ConnectionManager *mgr = ConnectionManager::instance();
            mgr->requestChannelMode(m_currentDevice, m_currentChannel, ConnectionManager::MODE_RX_ONLY, this, isFDWanted);
        }
    });
    
    // 筛选信号连接（使用FilterHeaderWidget的控件）
    connect(m_filterHeader->getSeqEdit(), &QLineEdit::textChanged, this, [this](const QString &text) {
        m_tableModel->setSeqFilter(text.trimmed());
        m_highlightDelegate->setHighlightKeyword(CANFrameTableModel::COL_SEQ, text.trimmed());
        m_tableView->viewport()->update();
    });
    
    connect(m_filterHeader->getTimeEdit(), &QLineEdit::textChanged, this, [this](const QString &text) {
        m_tableModel->setTimeFilter(text.trimmed());
        m_highlightDelegate->setHighlightKeyword(CANFrameTableModel::COL_TIME, text.trimmed());
        m_tableView->viewport()->update();
    });
    
    connect(m_filterHeader->getIDEdit(), &QLineEdit::textChanged, this, [this](const QString &text) {
        m_tableModel->setIDFilter(text.trimmed().toUpper());
        m_highlightDelegate->setHighlightKeyword(CANFrameTableModel::COL_ID, text.trimmed().toUpper());
        m_tableView->viewport()->update();
    });
    
    connect(m_filterHeader->getTypeCombo(), QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        m_tableModel->setTypeFilter(index);
        // 类型筛选不需要高亮（下拉框选择）
    });
    
    connect(m_filterHeader->getDirCombo(), QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        m_tableModel->setDirectionFilter(index);
        // 方向筛选不需要高亮（下拉框选择）
    });
    
    connect(m_filterHeader->getLenEdit(), &QLineEdit::textChanged, this, [this](const QString &text) {
        m_tableModel->setLengthFilter(text.trimmed());
        m_highlightDelegate->setHighlightKeyword(CANFrameTableModel::COL_LEN, text.trimmed());
        m_tableView->viewport()->update();
    });
    
    connect(m_filterHeader->getDataEdit(), &QLineEdit::textChanged, this, [this](const QString &text) {
        QString cleanText = text;
        cleanText.remove(' ');
        
        if (!cleanText.isEmpty() && cleanText.length() % 2 != 0) {
            m_filterHeader->getDataEdit()->setStyleSheet("background-color: #FFEEEE;");
        } else {
            m_filterHeader->getDataEdit()->setStyleSheet("background-color: white;");
        }
        
        m_tableModel->setDataFilter(text);
        m_highlightDelegate->setHighlightKeyword(CANFrameTableModel::COL_DATA, text.trimmed().toUpper());
        m_tableView->viewport()->update();
    });
    
    connect(m_filterHeader->getChannelEdit(), &QLineEdit::textChanged, this, [this](const QString &text) {
        bool ok;
        int channel = text.trimmed().toInt(&ok);
        if (ok || text.trimmed().isEmpty()) {
            m_tableModel->setChannelFilter(text.trimmed().isEmpty() ? -1 : channel);
            m_filterHeader->getChannelEdit()->setStyleSheet("background-color: white;");
            m_highlightDelegate->setHighlightKeyword(CANFrameTableModel::COL_CHANNEL, text.trimmed());
            m_tableView->viewport()->update();
        } else {
            m_filterHeader->getChannelEdit()->setStyleSheet("background-color: #FFEEEE;");
        }
    });
    
    // CANFrameTableModel 批量信号
    connect(m_tableModel, &CANFrameTableModel::framesReceived, this, 
        [this](const QVector<CANFrame> &frames) {
            // 实时保存（批量写入）
            if (m_realTimeSaver && m_realTimeSaver->isSaving()) {
                m_realTimeSaver->saveFrames(frames);
            }
        });
    
    // 按钮信号
    connect(m_btnPause, &QPushButton::clicked, this, &CANViewWidget::onPauseClicked);
    
    // 切换详情面板按钮
    connect(m_btnToggleDetail, &QPushButton::clicked, this, &CANViewWidget::onToggleDetailPanel);
    
    // 表格选择信号 - 显示详情
    connect(m_tableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, [this](const QModelIndex &current, const QModelIndex &) {
        if (current.isValid()) {
            // 获取选中行的数据
            quint64 seq = m_tableModel->data(m_tableModel->index(current.row(), CANFrameTableModel::COL_SEQ), Qt::DisplayRole).toULongLong();
            
            // 从模型获取完整的CANFrame
            // 注意：需要通过绝对索引获取
            quint64 absIndex = seq - 1;  // 序号从1开始，索引从0开始
            
            // 计算环形缓冲区索引
            int ringIndex = absIndex % m_tableModel->getCapacity();
            
            // 检查索引有效性
            quint64 oldestValid = 0;
            if (m_tableModel->getTotalFramesReceived() > (quint64)m_tableModel->getCapacity()) {
                oldestValid = m_tableModel->getTotalFramesReceived() - (quint64)m_tableModel->getCapacity();
            }
            
            if (absIndex >= oldestValid && absIndex < m_tableModel->getTotalFramesReceived()) {
                const CANFrame &frame = m_tableModel->getFrameByRingIndex(ringIndex);
                m_detailPanel->showFrame(frame, seq);
            } else {
                m_detailPanel->clear();
            }
        } else {
            m_detailPanel->clear();
        }
    });
    
    // 监听设备重连信号，重新下发配置
    connect(ConnectionManager::instance(), &ConnectionManager::deviceConnected,
            this, [this](const QString &port) {
        if (port == m_currentDevice && m_currentChannel >= -1) {
            qDebug() << "🔄 检测到设备重连: " << port << "，正在重新下发配置...";
            // 重新订阅以触发配置下发
            subscribe();
        }
    });

    // 达到容量上限后冻结自动刷新，仅在滚动到底部时手动刷新最新快照
    connect(m_tableView->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &CANViewWidget::onTableScrollChanged);
}

void CANViewWidget::setDevice(const QString &device)
{
    if (m_currentDevice == device) {
        return;
    }
    
    if (!m_currentDevice.isEmpty()) {
        unsubscribe();
    }
    
    m_currentDevice = device;
    
    if (m_currentChannel >= -1) {
        subscribe();
    }
}

void CANViewWidget::setChannel(int channel)
{
    if (m_currentChannel == channel) {
        return;
    }
    
    if (m_currentChannel >= -1 && !m_currentDevice.isEmpty()) {
        unsubscribe();
    }
    
    m_currentChannel = channel;
    
    if (!m_currentDevice.isEmpty() && m_currentChannel >= -1) {
        subscribe();
    }
}

void CANViewWidget::subscribe()
{
    if (m_currentDevice.isEmpty() || m_currentChannel < -1) {
        return;
    }
    
    DataRouter *router = DataRouter::instance();
    router->subscribe(this, m_currentDevice, m_currentChannel);
    
    disconnect(router, &DataRouter::framesRoutedToWidget, this, nullptr);
    
    // 显示层不直接接收路由帧，统一从存储层低频拉取，避免跨层回调拉扯
    
    // ✅ 启动ZCAN统计并请求通道模式
    ConnectionManager *mgr = ConnectionManager::instance();
    SerialConnection *conn = mgr->getConnection(m_currentDevice);
    if (conn) {
            // 1=CAN2.0, 2=CANFD
            bool isFDWanted = (m_comboCanType && m_comboCanType->currentIndex() == 2);
        // 🛑 检查冲突
        QString reason;
        bool conflict = false;
        if (m_currentChannel == -1) {
            if (!mgr->checkModeConflict(m_currentDevice, 0, ConnectionManager::MODE_RX_ONLY, reason) ||
                !mgr->checkModeConflict(m_currentDevice, 1, ConnectionManager::MODE_RX_ONLY, reason)) {
                conflict = true;
            }
        } else {
            if (!mgr->checkModeConflict(m_currentDevice, m_currentChannel, ConnectionManager::MODE_RX_ONLY, reason)) {
                conflict = true;
            }
        }

        if (conflict) {
            QMessageBox::warning(this, "模式冲突", reason + "\n请先停止发送/回放窗口。");
            m_comboChannel->blockSignals(true);
            m_comboChannel->setCurrentIndex(0);
            m_comboChannel->blockSignals(false);
            m_currentChannel = -2;
            return;
        }

        conn->startUCANStatistics();
        qDebug() << "📊 设备" << m_currentDevice << "统计已启动";
        
        // 🚀 使用 ConnectionManager 请求 RX 模式
        if (m_currentChannel == -1) {
            mgr->requestChannelMode(m_currentDevice, 0, ConnectionManager::MODE_RX_ONLY, this, isFDWanted);
            mgr->requestChannelMode(m_currentDevice, 1, ConnectionManager::MODE_RX_ONLY, this, isFDWanted);
        } else {
            mgr->requestChannelMode(m_currentDevice, m_currentChannel, ConnectionManager::MODE_RX_ONLY, this, isFDWanted);
        }
    }
    
    QString channelStr = (m_currentChannel == -1) ? "ALL" : QString("CAN%1").arg(m_currentChannel);
    qDebug() << "📡 窗口" << this << "订阅成功:" << m_currentDevice << "-" << channelStr;

    // 重置显示拉取游标，并确保UI轮询定时器运行
    m_lastPulledSequence = FrameStorageEngine::instance()->latestSequence();
    resetStatsBaseline();
    if (!m_uiUpdateTimer->isActive()) {
        m_uiUpdateTimer->start();
    }
}

void CANViewWidget::unsubscribe()
{
    DataRouter *router = DataRouter::instance();
    ConnectionManager *mgr = ConnectionManager::instance();
    
    // 释放通道模式申请
    if (!m_currentDevice.isEmpty()) {
        if (m_currentChannel == -1) {
            mgr->releaseChannelMode(m_currentDevice, 0, this);
            mgr->releaseChannelMode(m_currentDevice, 1, this);
        } else if (m_currentChannel >= 0) {
            mgr->releaseChannelMode(m_currentDevice, m_currentChannel, this);
        }
    }

    // 先取消订阅
    router->unsubscribe(this);
    
    disconnect(router, &DataRouter::framesRoutedToWidget, this, nullptr);
    
    // ⏸ 检查设备是否还有其他订阅者，如果没有则停止统计
    if (!m_currentDevice.isEmpty()) {
        if (!router->hasSubscribers(m_currentDevice)) {
            ConnectionManager *mgr = ConnectionManager::instance();
            SerialConnection *conn = mgr->getConnection(m_currentDevice);
            if (conn) {
                conn->stopUCANStatistics();
                qDebug() << "⏸ 设备" << m_currentDevice << "统计已停止（无订阅者）";
            }
        }
    }
    
    m_currentChannel = -2;
    m_lastPulledSequence = 0;
    
    qDebug() << "🔕 窗口" << this << "取消订阅";
}

void CANViewWidget::onFrameReceived(const CANFrame &frame)
{
    // 显示层不直接消费实时帧，统一由存储层拉取。
    Q_UNUSED(frame);
}

void CANViewWidget::onFramesReceived(const QVector<CANFrame> &frames)
{
    // 显示层不直接消费实时帧，统一由存储层拉取。
    Q_UNUSED(frames);
}

void CANViewWidget::onClearClicked()
{
    m_tableModel->clearFrames();
    m_pendingFrames.clear();
    m_uiFrozenAfterCap = false;
    m_frozenDataDirty = false;
    m_isApplyingFrozenSnapshot = false;
    m_lastPulledSequence = FrameStorageEngine::instance()->latestSequence();
    if (m_lblFreezeHint) {
        m_lblFreezeHint->setText("实时刷新中");
        m_lblFreezeHint->setStyleSheet("QLabel { color: #455A64; }");
    }
    
    // 统计改为真实累计口径：清空时仅重置显示基线，不修改底层累计值。
    resetStatsBaseline();
    updateStatusBar();
    
    qDebug() << "🗑 窗口" << this << "清空数据";
}

void CANViewWidget::onPauseClicked()
{
    m_isPaused = !m_isPaused;
    
    if (m_isPaused) {
        m_btnPause->setText(QString::fromUtf8("继续"));
        m_btnPause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    } else {
        m_btnPause->setText(QString::fromUtf8("暂停"));
        m_btnPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    }
}

void CANViewWidget::onSaveClicked()
{
    if (m_exportInProgress) {
        QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("正在导出，请稍候"));
        return;
    }

    QString defaultFileName = QString("CAN_数据_%1.xlsx")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    
    QString selectedFilter;
    QString fileName = QFileDialog::getSaveFileName(
        this,
        QString::fromUtf8("保存CAN数据"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + defaultFileName,
        QString::fromUtf8("Excel文件 (*.xlsx);;ASC文件 (*.asc);;CSV文件 (*.csv);;TXT文本 (*.txt);;所有文件 (*.*)"),
        &selectedFilter,
        QFileDialog::Options()
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // 根据选择的过滤器自动添加正确的扩展名
    QFileInfo fileInfo(fileName);
    QString baseName = fileInfo.completeBaseName();
    QString dirPath = fileInfo.absolutePath();
    
    if (selectedFilter.contains("Excel") || selectedFilter.contains("xlsx")) {
        if (!fileName.endsWith(".xlsx", Qt::CaseInsensitive)) {
            fileName = dirPath + "/" + baseName + ".xlsx";
        }
    } else if (selectedFilter.contains("ASC") || selectedFilter.contains("asc")) {
        if (!fileName.endsWith(".asc", Qt::CaseInsensitive)) {
            fileName = dirPath + "/" + baseName + ".asc";
        }
    } else if (selectedFilter.contains("CSV") || selectedFilter.contains("csv")) {
        if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
            fileName = dirPath + "/" + baseName + ".csv";
        }
    } else if (selectedFilter.contains("TXT") || selectedFilter.contains("txt")) {
        if (!fileName.endsWith(".txt", Qt::CaseInsensitive)) {
            fileName = dirPath + "/" + baseName + ".txt";
        }
    }
    
    QVector<CANFrame> frames;
    if (!m_currentDevice.isEmpty() && m_currentChannel >= -1) {
        frames = FrameStorageEngine::instance()->snapshotFrames(m_currentDevice, m_currentChannel, 300000);
    }
    if (frames.isEmpty()) {
        frames = m_tableModel->getAllFrames();
    }
    m_exportInProgress = true;
    setCursor(Qt::BusyCursor);

    QPointer<CANViewWidget> guard(this);
    QThread *worker = QThread::create([guard, frames = std::move(frames), fileName, selectedFilter]() mutable {
        ExportTaskResult result = exportFramesToFile(frames, fileName, selectedFilter);
        if (!guard) {
            return;
        }
        QMetaObject::invokeMethod(guard, [guard, result]() {
            if (!guard) {
                return;
            }
            guard->m_exportInProgress = false;
            guard->unsetCursor();
            if (result.ok) {
                QMessageBox::information(guard, QString::fromUtf8("完成"),
                    QString::fromUtf8("已导出 %1 条数据到：\n%2")
                    .arg(result.frameCount)
                    .arg(result.path));
            } else {
                QMessageBox::critical(guard, QString::fromUtf8("错误"), result.error);
            }
        }, Qt::QueuedConnection);
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void CANViewWidget::onRealTimeSaveClicked()
{
    if (!m_realTimeSaver->isSaving()) {
        QString defaultFileName = QString("CAN_录制_%1.blf") // 默认改为 .blf
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
        
        QString filePath = QFileDialog::getSaveFileName(
            this,
            QString::fromUtf8("选择实时保存路径"),
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + defaultFileName,
            // 调整过滤器顺序：BLF在前
            QString::fromUtf8("BLF文件 (*.blf);;ASC文件 (*.asc);;CAN文件 (*.can);;所有文件 (*.*)"),
            nullptr,
            QFileDialog::Options()
        );
        
        if (filePath.isEmpty()) {
            return;
        }
        
        QString ext = QFileInfo(filePath).suffix().toLower();
        CANViewRealTimeSaver::SaveFormat format;
        
        if (ext == "asc") {
            format = CANViewRealTimeSaver::FormatASC;
        } else if (ext == "blf") {
            format = CANViewRealTimeSaver::FormatBLF;
        } else if (ext == "can") {
            format = CANViewRealTimeSaver::FormatCAN;
        } else {
            // 如果没有扩展名或未知的，默认用BLF
            format = CANViewRealTimeSaver::FormatBLF;
            if (ext.isEmpty()) {
                filePath += ".blf";
            }
        }
        
        if (m_realTimeSaver->startSaving(filePath, format)) {
            m_btnRealTimeSave->setText(QString::fromUtf8("停止保存"));
            m_btnRealTimeSave->setIcon(style()->standardIcon(QStyle::SP_MediaStop));  // 停止图标
            m_btnRealTimeSave->setStyleSheet("background-color: #FF5252; color: white;");
        } else {
            QMessageBox::critical(this, QString::fromUtf8("错误"), QString::fromUtf8("无法创建文件：") + filePath);
        }
        
    } else {
        m_realTimeSaver->stopSaving();
        
        m_btnRealTimeSave->setText(QString::fromUtf8("实时保存"));
        m_btnRealTimeSave->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // 恢复播放/录制图标
        m_btnRealTimeSave->setStyleSheet("");
        
        QMessageBox::information(this, QString::fromUtf8("完成"), QString::fromUtf8("实时保存已停止"));
    }
}

void CANViewWidget::updateStatusBar()
{
    FrameStorageEngine::StorageStats stats{};
    if (!m_currentDevice.isEmpty() && m_currentChannel >= -1) {
        stats = FrameStorageEngine::instance()->queryStats(m_currentDevice, m_currentChannel);
    }

    const quint64 rx = (stats.rxFrames >= m_rxBase) ? (stats.rxFrames - m_rxBase) : 0;
    const quint64 tx = (stats.txFrames >= m_txBase) ? (stats.txFrames - m_txBase) : 0;
    const quint64 err = (stats.errorFrames >= m_errorBase) ? (stats.errorFrames - m_errorBase) : 0;

    m_rxCount = rx;
    m_txCount = tx;
    m_errorCount = err;

    m_lblRxCount->setText(QString("接收帧计数: %1").arg(rx));
    m_lblTxCount->setText(QString("发送帧计数: %1").arg(tx));
    m_lblErrCount->setText(QString("错误帧计数: %1").arg(err));
}

void CANViewWidget::updateDeviceList()
{
    // 保存当前选择的设备和通道
    QString savedDevice = m_currentDevice;
    int savedChannel = m_currentChannel;
    
    // 阻止信号，避免在更新列表时触发取消订阅
    m_comboDevice->blockSignals(true);
    m_comboChannel->blockSignals(true);
    m_comboCanType->blockSignals(true);
    
    m_comboDevice->clear();
    m_comboDevice->addItem("请选择设备");
    
    ConnectionManager *mgr = ConnectionManager::instance();
    QStringList devices = mgr->connectedDevices();
    
    for (const QString &device : devices) {
        m_comboDevice->addItem(device);
    }
    
    if (devices.isEmpty()) {
        m_comboDevice->addItem("(无可用设备，请先连接)");
    }
    
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(m_comboDevice->model());
    if (model) {
        QStandardItem *item0 = model->item(0);
        if (item0) {
            item0->setFlags(item0->flags() & ~Qt::ItemIsEnabled);
            item0->setData(QColor(Qt::gray), Qt::ForegroundRole);
        }
        
        if (devices.isEmpty() && m_comboDevice->count() > 1) {
            QStandardItem *itemLast = model->item(m_comboDevice->count() - 1);
            if (itemLast) {
                itemLast->setFlags(itemLast->flags() & ~Qt::ItemIsEnabled);
                itemLast->setData(QColor(Qt::gray), Qt::ForegroundRole);
            }
        }
    }
    
    // 尝试恢复之前的选择
    if (!savedDevice.isEmpty()) {
        int index = m_comboDevice->findText(savedDevice);
        if (index != -1) {
            // 找到了之前选择的设备，恢复UI显示（不影响订阅）
            m_comboDevice->setCurrentIndex(index);
            
            // 恢复通道选择（须先恢复 CAN 类型占位，再启用通道）
            if (savedChannel >= -1) {
                m_comboCanType->setEnabled(true);
                m_comboCanType->setCurrentIndex(1);  // 默认 CAN2.0；列表刷新后用户可改 FD
                m_comboChannel->setEnabled(true);
                if (savedChannel == -1) {
                    m_comboChannel->setCurrentIndex(1);  // ALL (所有通道)
                    qDebug() << "✅ 恢复通道选择: ALL (订阅保持不变)";
                } else {
                    m_comboChannel->setCurrentIndex(savedChannel + 2);  // CAN0=2, CAN1=3
                    qDebug() << "✅ 恢复通道选择: CAN" << savedChannel << " (订阅保持不变)";
                }
                
                // 🚀 修复：确保统计模块已启动
                // 在重连场景下，由于blockSignals，onDeviceChanged不会被触发，
                // 导致subscribe()不会被重新调用，进而导致新连接的统计模块未启动。
                // 这里显式调用一次startUCANStatistics。
                ConnectionManager *mgr = ConnectionManager::instance();
                SerialConnection *conn = mgr->getConnection(savedDevice);
                if (conn) {
                    conn->startUCANStatistics();
                    qDebug() << "📊 视图恢复设备选择，强制启动统计:" << savedDevice;
                }
            }
        } else {
            // 设备已断开，清理状态
            qDebug() << "⚠️ 设备已断开:" << savedDevice;
            m_currentDevice = "";
            m_currentChannel = -2;
            m_comboCanType->setEnabled(false);
            m_comboCanType->setCurrentIndex(0);
            m_comboChannel->setEnabled(false);
            m_comboChannel->setCurrentIndex(0);
            
            // 恢复信号后再取消订阅
            m_comboDevice->blockSignals(false);
            m_comboChannel->blockSignals(false);
            m_comboCanType->blockSignals(false);
            unsubscribe();
            return;
        }
    }
    
    // 恢复信号
    m_comboDevice->blockSignals(false);
    m_comboChannel->blockSignals(false);
    m_comboCanType->blockSignals(false);
}

void CANViewWidget::loadSettings()
{
    QSettings settings("CANTools", "CANAnalyzerPro");
    
    m_timeFormat = settings.value("CANView/TimeFormat", 0).toInt();
    m_dataFormat = settings.value("CANView/DataFormat", 0).toInt();
    m_idFormat = settings.value("CANView/IDFormat", 0).toInt();
    
    settings.beginGroup("IDFilter");
    m_idFilterRule.enabled = settings.value("enabled", false).toBool();
    m_idFilterRule.mode = static_cast<IDFilterRule::FilterMode>(settings.value("mode", IDFilterRule::FilterOnDisplay).toInt());
    m_idFilterRule.isHexMode = settings.value("isHexMode", true).toBool();
    m_idFilterRule.compareOp1 = static_cast<IDFilterRule::CompareOp>(settings.value("compareOp1", 0).toInt());
    m_idFilterRule.value1 = settings.value("value1", "").toString();
    m_idFilterRule.logicOp = static_cast<IDFilterRule::LogicOp>(settings.value("logicOp", 1).toInt());
    m_idFilterRule.compareOp2 = static_cast<IDFilterRule::CompareOp>(settings.value("compareOp2", 0).toInt());
    m_idFilterRule.value2 = settings.value("value2", "").toString();
    settings.endGroup();
    
    // 加载并应用列显示设置
    QMap<int, bool> columnVisibility;
    columnVisibility[CANFrameTableModel::COL_SEQ] = settings.value("CANView/ShowSeq", true).toBool();
    columnVisibility[CANFrameTableModel::COL_TIME] = settings.value("CANView/ShowTime", true).toBool();
    columnVisibility[CANFrameTableModel::COL_CHANNEL] = settings.value("CANView/ShowChannel", true).toBool();
    columnVisibility[CANFrameTableModel::COL_ID] = settings.value("CANView/ShowID", true).toBool();
    columnVisibility[CANFrameTableModel::COL_TYPE] = settings.value("CANView/ShowType", true).toBool();
    columnVisibility[CANFrameTableModel::COL_DIR] = settings.value("CANView/ShowDir", true).toBool();
    columnVisibility[CANFrameTableModel::COL_LEN] = settings.value("CANView/ShowLen", true).toBool();
    columnVisibility[CANFrameTableModel::COL_DATA] = settings.value("CANView/ShowData", true).toBool();
    
    if (m_tableModel) {
        m_tableModel->setTimeFormat(m_timeFormat);
        m_tableModel->setDataFormat(m_dataFormat);
        m_tableModel->setIDFormat(m_idFormat);
        m_tableModel->setStartTime(m_startTime);
        
        // ✅ 应用ID过滤规则到模型
        m_tableModel->setIDFilterRule(m_idFilterRule);
    }
    
    // 更新过滤状态显示
    if (m_lblFilterStatus) {
        m_lblFilterStatus->setVisible(m_idFilterRule.enabled);
    }
    
    // 直接应用列显示设置（UI已经在setupUi中创建完成）
    applyColumnVisibility(columnVisibility);
}

void CANViewWidget::applySettings()
{
    // 更新表格模型的格式设置
    m_tableModel->setTimeFormat(m_timeFormat);
    m_tableModel->setDataFormat(m_dataFormat);
    m_tableModel->setIDFormat(m_idFormat);
    
    // 更新详情面板的格式设置
    if (m_detailPanel) {
        m_detailPanel->setDataFormat(m_dataFormat);
        m_detailPanel->setIDFormat(m_idFormat);
    }
    
    // 刷新显示（触发重绘）
    m_tableView->viewport()->update();
    
    qDebug() << "✅ 设置已应用";
}

void CANViewWidget::applyColumnVisibility(const QMap<int, bool> &visibility)
{
    for (auto it = visibility.constBegin(); it != visibility.constEnd(); ++it) {
        int column = it.key();
        bool visible = it.value();
        m_tableView->setColumnHidden(column, !visible);
    }
    
    // 更新筛选头部的列宽
    if (m_filterHeader) {
        m_filterHeader->updateColumnWidths();
    }
    
    qDebug() << "✅ 列显示设置已应用";
}

void CANViewWidget::onSettingsClicked()
{
    CANViewSettingsDialog dialog(this);
    
    dialog.setFilterRule(m_idFilterRule);
    
    if (dialog.exec() == QDialog::Accepted) {
        m_timeFormat = dialog.getTimeFormat();
        m_dataFormat = dialog.getDataFormat();
        m_idFormat = dialog.getIDFormat();
        
        // 获取新的过滤规则
        IDFilterRule newFilterRule = dialog.getFilterRule();
        
        // 检查过滤模式是否改变
        bool modeChanged = (m_idFilterRule.mode != newFilterRule.mode);
        bool wasReceiveMode = (m_idFilterRule.mode == IDFilterRule::FilterOnReceive);
        bool isReceiveMode = (newFilterRule.mode == IDFilterRule::FilterOnReceive);
        
        // 保存新规则
        m_idFilterRule = newFilterRule;
        
        // 应用到TableModel
        if (m_tableModel) {
            m_tableModel->setIDFilterRule(m_idFilterRule);
        }
        
        // 更新过滤状态显示
        if (m_lblFilterStatus) {
            m_lblFilterStatus->setVisible(m_idFilterRule.enabled);
        }
        
        // 如果从接收时过滤切换到显示时过滤，需要清空数据重新接收
        if (modeChanged && wasReceiveMode && !isReceiveMode) {
            onClearClicked();
        }
        
        // 应用新设置
        applySettings();
        
        // 应用列显示设置
        QMap<int, bool> columnVisibility = dialog.getColumnVisibility();
        applyColumnVisibility(columnVisibility);
    }
}



void CANViewWidget::onToggleDetailPanel()
{
    // 切换详情面板的显示/隐藏
    bool isVisible = m_detailPanel->isVisible();
    
    if (isVisible) {
        // 隐藏详情面板（按钮保持可见）
        m_detailPanel->hide();
        m_btnToggleDetail->setText(QString::fromUtf8("◀"));  // 隐藏后显示◀（向左箭头）
        m_btnToggleDetail->setToolTip(QString::fromUtf8("显示详情面板"));
        qDebug() << "🔽 详情面板已隐藏";
    } else {
        // 显示详情面板
        m_detailPanel->show();
        m_btnToggleDetail->setText(QString::fromUtf8("▶"));  // 显示后显示▶（向右箭头）
        m_btnToggleDetail->setToolTip(QString::fromUtf8("隐藏详情面板"));
        qDebug() << "🔼 详情面板已显示";
        
        // 如果有选中的行，立即显示详情
        QModelIndex current = m_tableView->currentIndex();
        if (current.isValid()) {
            quint64 seq = m_tableModel->data(m_tableModel->index(current.row(), CANFrameTableModel::COL_SEQ), Qt::DisplayRole).toULongLong();
            quint64 absIndex = seq - 1;
            int ringIndex = absIndex % m_tableModel->getCapacity();
            
            // 检查索引有效性
            quint64 oldestValid = 0;
            if (m_tableModel->getTotalFramesReceived() > (quint64)m_tableModel->getCapacity()) {
                oldestValid = m_tableModel->getTotalFramesReceived() - (quint64)m_tableModel->getCapacity();
            }
            
            if (absIndex >= oldestValid && absIndex < m_tableModel->getTotalFramesReceived()) {
                const CANFrame &frame = m_tableModel->getFrameByRingIndex(ringIndex);
                m_detailPanel->showFrame(frame, seq);
            }
        }
    }
}

void CANViewWidget::onUIUpdateTimeout()
{
    // 周期性更新自适应策略（基于实际拉取到的帧速率）
    if (m_frameRateTimer.elapsed() >= 1000) {
        updateAdaptiveStrategy();
        m_frameCountInSecond = 0;
        m_frameRateTimer.restart();
    }

    // 显示层低频拉取：仅从存储层获取增量帧，不直接接收路由层回调
    QVector<CANFrame> pulledFrames;
    quint64 latestPulledSeq = m_lastPulledSequence;
    const int pullLimit = qMax(5000, m_adaptiveBatchSize * 16);
    if (!m_currentDevice.isEmpty() && m_currentChannel >= -1) {
        pulledFrames = FrameStorageEngine::instance()->snapshotFramesSince(
            m_currentDevice, m_currentChannel, m_lastPulledSequence, pullLimit, &latestPulledSeq);
    }
    m_lastPulledSequence = latestPulledSeq;

    if (!pulledFrames.isEmpty()) {
        m_frameCountInSecond += pulledFrames.size();
        if (m_uiFrozenAfterCap) {
            m_frozenDataDirty = true;
        } else {
            m_pendingFrames.append(pulledFrames);
        }
    }

    // 拉取后判断是否进入冻结
    if (!m_uiFrozenAfterCap &&
        (m_tableModel->getTotalFramesReceived() + static_cast<quint64>(m_pendingFrames.size()) >= UI_FREEZE_THRESHOLD)) {
        m_uiFrozenAfterCap = true;
        m_frozenDataDirty = true;
        m_pendingFrames.clear();
        if (m_lblFreezeHint) {
            m_lblFreezeHint->setText("已冻结，滚动到底部刷新最新数据");
            m_lblFreezeHint->setStyleSheet("QLabel { color: #D84315; font-weight: bold; }");
        }
    }

    if (m_uiFrozenAfterCap) {
        // 冻结模式下只更新计数和状态，不更新表格
        updateStatusBar();
        return;
    }

    // UI更新定时器超时 - 批量刷新UI（显示层拉取模式下，订阅期间保持轮询）
    if (m_pendingFrames.isEmpty()) {
        // 仅在未订阅状态停表，避免“无数据瞬间”把拉取循环停死。
        if (m_currentDevice.isEmpty() || m_currentChannel < -1) {
            m_uiUpdateTimer->stop();
        }
        return;
    }
    
    // 批量添加到表格模型
    m_tableModel->addFrames(m_pendingFrames);
    m_pendingFrames.clear();
    
    // 触发视图更新
    QModelIndex topLeft = m_tableView->indexAt(m_tableView->rect().topLeft());
    QModelIndex bottomRight = m_tableView->indexAt(m_tableView->rect().bottomRight());
    if (topLeft.isValid() && bottomRight.isValid()) {
        emit m_tableModel->dataChanged(topLeft, bottomRight);
    }
    
    // 更新状态栏
    updateStatusBar();
}

void CANViewWidget::onTableScrollChanged(int value)
{
    if (!m_uiFrozenAfterCap || !m_frozenDataDirty || m_isApplyingFrozenSnapshot) {
        return;
    }

    QScrollBar *bar = m_tableView->verticalScrollBar();
    if (!bar) {
        return;
    }

    if (value >= bar->maximum()) {
        refreshFrozenUiSnapshot();
    }
}

void CANViewWidget::refreshFrozenUiSnapshot()
{
    if (m_isApplyingFrozenSnapshot) {
        return;
    }

    m_isApplyingFrozenSnapshot = true;

    QVector<CANFrame> latestFrames;
    if (!m_currentDevice.isEmpty() && m_currentChannel >= -1) {
        latestFrames = FrameStorageEngine::instance()->snapshotFrames(m_currentDevice, m_currentChannel, UI_FREEZE_THRESHOLD);
    } else {
        latestFrames = m_tableModel->getAllFrames();
    }

    m_tableModel->clearFrames();
    if (!latestFrames.isEmpty()) {
        m_tableModel->addFrames(latestFrames);
    }
    m_lastPulledSequence = FrameStorageEngine::instance()->latestSequence();

    // 每次手动刷新后把滚动条放回顶部，必须再次拖到底部才触发下次刷新
    m_tableView->scrollToTop();
    m_frozenDataDirty = false;
    m_isApplyingFrozenSnapshot = false;
    if (m_lblFreezeHint) {
        m_lblFreezeHint->setText("已刷新，继续滚动到底部可再次刷新");
        m_lblFreezeHint->setStyleSheet("QLabel { color: #2E7D32; font-weight: bold; }");
    }
}

// 🚀 阶段1优化：自适应批量策略实现
void CANViewWidget::updateAdaptiveStrategy()
{
    int frameRate = m_frameCountInSecond;
    
    // 根据帧率动态调整批量大小和UI更新间隔
    if (frameRate > 5000) {
        // 极高速：>5000帧/秒
        m_adaptiveBatchSize = 800;
        m_adaptiveUIInterval = 350;  // 2.8fps
    } else if (frameRate > 2000) {
        // 高速：2000-5000帧/秒
        m_adaptiveBatchSize = 500;
        m_adaptiveUIInterval = 250;  // 4fps
    } else if (frameRate > 500) {
        // 中速：500-2000帧/秒
        m_adaptiveBatchSize = 300;
        m_adaptiveUIInterval = 150;  // 6.7fps
    } else {
        // 低速：<500帧/秒
        m_adaptiveBatchSize = 150;
        m_adaptiveUIInterval = 100;  // 10fps
    }
    
    // 应用新的UI更新间隔
    m_uiUpdateTimer->setInterval(m_adaptiveUIInterval);
    
#if CANVIEW_ADAPTIVE_STRATEGY_LOGGING
    qDebug() << "📊 自适应策略更新: 帧率=" << frameRate 
             << "fps, 批量大小=" << m_adaptiveBatchSize 
             << ", UI间隔=" << m_adaptiveUIInterval << "ms";
#endif
}

void CANViewWidget::resetStatsBaseline()
{
    if (m_currentDevice.isEmpty() || m_currentChannel < -1) {
        m_rxBase = 0;
        m_txBase = 0;
        m_errorBase = 0;
        return;
    }

    const auto stats = FrameStorageEngine::instance()->queryStats(m_currentDevice, m_currentChannel);
    m_rxBase = stats.rxFrames;
    m_txBase = stats.txFrames;
    m_errorBase = stats.errorFrames;
}
