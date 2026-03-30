#include "StatisticsDialog.h"
#include "../core/SerialConnection.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

StatisticsDialog::StatisticsDialog(const QMap<QString, SerialConnection*> &connections, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("性能统计");
    setMinimumSize(520, 520);
    // 设置窗口标志：对话框 + 关闭按钮 + 最大化按钮 (无最小化)
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);
    
    // 为每个设备创建统计页面
    for (auto it = connections.constBegin(); it != connections.constEnd(); ++it) {
        QString deviceName = it.key();
        SerialConnection *conn = it.value();
        
        if (conn) {
            ZCAN::ZCANStatistics *stats = conn->getUCANStatistics();
            if (stats) {
                DeviceStatsPage page;
                page.deviceName = deviceName;
                page.statistics = stats;
                page.connection = conn;
                page.pageWidget = nullptr;  // 先设置为nullptr
                page.lastFwRequestMs = 0;
                m_devicePages.append(page);  // 先添加到列表
                // 然后创建页面（此时可以访问m_devicePages.last()）
                m_devicePages.last().pageWidget = createDevicePage(deviceName, stats);
            }
        }
    }
    
    setupUi();
    
    // 创建更新定时器（每500ms更新一次）
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &StatisticsDialog::updateStatistics);
    m_updateTimer->start(500);
    
    // 立即更新一次
    updateStatistics();
}

StatisticsDialog::~StatisticsDialog()
{
}

void StatisticsDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // 创建标签页控件
    m_tabWidget = new QTabWidget();
    
    // 添加所有设备页面
    for (int i = 0; i < m_devicePages.size(); i++) {
        m_tabWidget->addTab(m_devicePages[i].pageWidget, m_devicePages[i].deviceName);
    }
    
    // 如果没有设备，显示提示
    if (m_devicePages.isEmpty()) {
        QWidget *emptyPage = new QWidget();
        QVBoxLayout *emptyLayout = new QVBoxLayout(emptyPage);
        QLabel *emptyLabel = new QLabel("没有连接的设备");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: gray; font-size: 14px;");
        emptyLayout->addWidget(emptyLabel);
        m_tabWidget->addTab(emptyPage, "无设备");
    }
    
    mainLayout->addWidget(m_tabWidget);
    
    // 底部按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton *btnReset = new QPushButton("重置当前设备");
    QPushButton *btnResetAll = new QPushButton("重置所有设备");
    QPushButton *btnExport = new QPushButton("导出CSV");
    QPushButton *btnClose = new QPushButton("关闭");
    
    connect(btnReset, &QPushButton::clicked, this, &StatisticsDialog::onResetClicked);
    connect(btnResetAll, &QPushButton::clicked, this, [this]() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "确认重置",
            "确定要重置所有设备的统计数据吗？",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            for (auto &page : m_devicePages) {
                if (page.statistics) {
                    page.statistics->reset();
                }
            }
            updateStatistics();
        }
    });
    connect(btnExport, &QPushButton::clicked, this, &StatisticsDialog::onExportClicked);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    
    buttonLayout->addWidget(btnReset);
    buttonLayout->addWidget(btnResetAll);
    buttonLayout->addWidget(btnExport);
    buttonLayout->addWidget(btnClose);
    
    mainLayout->addLayout(buttonLayout);
}

QWidget* StatisticsDialog::createDevicePage(const QString & /*deviceName*/, ZCAN::ZCANStatistics * /*statistics*/)
{
    QWidget *pageWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(pageWidget);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);
    
    // 创建一个临时的DeviceStatsPage来初始化标签
    DeviceStatsPage &page = m_devicePages.last();

    QScrollArea *scrollArea = new QScrollArea(pageWidget);
    scrollArea->setWidgetResizable(true);

    QWidget *contentWidget = new QWidget(scrollArea);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(10);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    contentLayout->addWidget(createThroughputGroup(page));
    contentLayout->addWidget(createLatencyGroup(page));
    contentLayout->addWidget(createBatchGroup(page));
    contentLayout->addWidget(createCompressionGroup(page));
    contentLayout->addWidget(createErrorGroup(page));
    contentLayout->addWidget(createFirmwareGroup(page));
    contentLayout->addWidget(createRuntimeGroup(page));
    contentLayout->addStretch();

    scrollArea->setWidget(contentWidget);
    layout->addWidget(scrollArea);
    
    return pageWidget;
}

QGroupBox* StatisticsDialog::createThroughputGroup(DeviceStatsPage &page)
{
    QGroupBox *group = new QGroupBox("吞吐量指标");
    QGridLayout *layout = new QGridLayout(group);
    
    page.lblFramesReceived = new QLabel("0 帧");
    page.lblFramesSent = new QLabel("0 帧");
    page.lblThroughputFps = new QLabel("0 帧/秒");
    page.lblThroughputMbps = new QLabel("0.00 Mbps");
    
    layout->addWidget(new QLabel("接收帧数："), 0, 0);
    layout->addWidget(page.lblFramesReceived, 0, 1);
    layout->addWidget(new QLabel("发送帧数："), 1, 0);
    layout->addWidget(page.lblFramesSent, 1, 1);
    layout->addWidget(new QLabel("吞吐量："), 2, 0);
    layout->addWidget(page.lblThroughputFps, 2, 1);
    layout->addWidget(new QLabel(""), 3, 0);
    layout->addWidget(page.lblThroughputMbps, 3, 1);
    
    return group;
}

QGroupBox* StatisticsDialog::createLatencyGroup(DeviceStatsPage &page)
{
    QGroupBox *group = new QGroupBox("延迟指标");
    QGridLayout *layout = new QGridLayout(group);
    
    page.lblAvgLatency = new QLabel("-- μs");
    page.lblMinLatency = new QLabel("-- μs");
    page.lblMaxLatency = new QLabel("-- μs");
    
    layout->addWidget(new QLabel("平均延迟："), 0, 0);
    layout->addWidget(page.lblAvgLatency, 0, 1);
    layout->addWidget(new QLabel("最小延迟："), 1, 0);
    layout->addWidget(page.lblMinLatency, 1, 1);
    layout->addWidget(new QLabel("最大延迟："), 2, 0);
    layout->addWidget(page.lblMaxLatency, 2, 1);
    
    return group;
}

QGroupBox* StatisticsDialog::createBatchGroup(DeviceStatsPage &page)
{
    QGroupBox *group = new QGroupBox("批量传输指标");
    QGridLayout *layout = new QGridLayout(group);
    
    page.lblBatchCount = new QLabel("0 次");
    page.lblAvgBatchSize = new QLabel("0.0 帧/批");
    page.lblBatchEfficiency = new QLabel("0.0%");
    
    layout->addWidget(new QLabel("批量次数："), 0, 0);
    layout->addWidget(page.lblBatchCount, 0, 1);
    layout->addWidget(new QLabel("平均批量："), 1, 0);
    layout->addWidget(page.lblAvgBatchSize, 1, 1);
    layout->addWidget(new QLabel("批量效率："), 2, 0);
    layout->addWidget(page.lblBatchEfficiency, 2, 1);
    
    return group;
}

QGroupBox* StatisticsDialog::createCompressionGroup(DeviceStatsPage &page)
{
    QGroupBox *group = new QGroupBox("压缩指标");
    QGridLayout *layout = new QGridLayout(group);
    
    page.lblCompressCount = new QLabel("0 次");
    page.lblOriginalSize = new QLabel("0 B");
    page.lblCompressedSize = new QLabel("0 B");
    page.lblCompressionRatio = new QLabel("0.0%");
    
    layout->addWidget(new QLabel("压缩次数："), 0, 0);
    layout->addWidget(page.lblCompressCount, 0, 1);
    layout->addWidget(new QLabel("原始大小："), 1, 0);
    layout->addWidget(page.lblOriginalSize, 1, 1);
    layout->addWidget(new QLabel("压缩后："), 2, 0);
    layout->addWidget(page.lblCompressedSize, 2, 1);
    layout->addWidget(new QLabel("压缩率："), 3, 0);
    layout->addWidget(page.lblCompressionRatio, 3, 1);
    
    return group;
}

QGroupBox* StatisticsDialog::createErrorGroup(DeviceStatsPage &page)
{
    QGroupBox *group = new QGroupBox("错误指标");
    QGridLayout *layout = new QGridLayout(group);
    
    page.lblFramesError = new QLabel("0 帧");
    page.lblFramesDropped = new QLabel("0 帧");
    page.lblErrorRate = new QLabel("0.00%");
    
    layout->addWidget(new QLabel("错误帧数："), 0, 0);
    layout->addWidget(page.lblFramesError, 0, 1);
    layout->addWidget(new QLabel("丢帧数："), 1, 0);
    layout->addWidget(page.lblFramesDropped, 1, 1);
    layout->addWidget(new QLabel("错误率："), 2, 0);
    layout->addWidget(page.lblErrorRate, 2, 1);
    
    return group;
}

QGroupBox* StatisticsDialog::createRuntimeGroup(DeviceStatsPage &page)
{
    QGroupBox *group = new QGroupBox("运行状态");
    QGridLayout *layout = new QGridLayout(group);
    
    page.lblUptime = new QLabel("00:00:00");
    page.lblCurrentMode = new QLabel("--");
    
    layout->addWidget(new QLabel("运行时间："), 0, 0);
    layout->addWidget(page.lblUptime, 0, 1);
    layout->addWidget(new QLabel("当前模式："), 1, 0);
    layout->addWidget(page.lblCurrentMode, 1, 1);
    
    return group;
}

QGroupBox* StatisticsDialog::createFirmwareGroup(DeviceStatsPage &page)
{
    QGroupBox *group = new QGroupBox("固件统计");
    QGridLayout *layout = new QGridLayout(group);

    page.lblFwCan1Rx = new QLabel("--");
    page.lblFwCan1Tx = new QLabel("--");
    page.lblFwCan1Err = new QLabel("--");
    page.lblFwCan2Rx = new QLabel("--");
    page.lblFwCan2Tx = new QLabel("--");
    page.lblFwCan2Err = new QLabel("--");
    page.lblFwTotalRx = new QLabel("--");
    page.lblFwTotalBatches = new QLabel("--");
    page.lblFwUpdatedAt = new QLabel("--");

    layout->addWidget(new QLabel("CAN1 RX："), 0, 0);
    layout->addWidget(page.lblFwCan1Rx, 0, 1);
    layout->addWidget(new QLabel("CAN1 TX："), 1, 0);
    layout->addWidget(page.lblFwCan1Tx, 1, 1);
    layout->addWidget(new QLabel("CAN1 ERR："), 2, 0);
    layout->addWidget(page.lblFwCan1Err, 2, 1);

    layout->addWidget(new QLabel("CAN2 RX："), 3, 0);
    layout->addWidget(page.lblFwCan2Rx, 3, 1);
    layout->addWidget(new QLabel("CAN2 TX："), 4, 0);
    layout->addWidget(page.lblFwCan2Tx, 4, 1);
    layout->addWidget(new QLabel("CAN2 ERR："), 5, 0);
    layout->addWidget(page.lblFwCan2Err, 5, 1);

    layout->addWidget(new QLabel("总RX："), 6, 0);
    layout->addWidget(page.lblFwTotalRx, 6, 1);
    layout->addWidget(new QLabel("总批次："), 7, 0);
    layout->addWidget(page.lblFwTotalBatches, 7, 1);

    layout->addWidget(new QLabel("更新时间："), 8, 0);
    layout->addWidget(page.lblFwUpdatedAt, 8, 1);

    return group;
}

void StatisticsDialog::updateStatistics()
{
    // 更新所有设备的统计
    for (auto &page : m_devicePages) {
        updateDeviceStatistics(page);
    }
}

void StatisticsDialog::updateDeviceStatistics(DeviceStatsPage &page)
{
    if (!page.statistics) {
        return;
    }
    
    ZCAN::PerformanceMetrics metrics = page.statistics->getMetrics();
    
    // 更新吞吐量指标
    page.lblFramesReceived->setText(QString::number(metrics.framesReceived) + " 帧");
    page.lblFramesSent->setText(QString::number(metrics.framesSent) + " 帧");
    page.lblThroughputFps->setText(QString::number(metrics.throughputFps, 'f', 0) + " 帧/秒");
    page.lblThroughputMbps->setText(QString::number(metrics.throughputMbps, 'f', 2) + " Mbps");
    
    // 更新延迟指标
    if (metrics.avgLatencyUs > 0) {
        page.lblAvgLatency->setText(QString::number(metrics.avgLatencyUs, 'f', 1) + " μs");
        page.lblMinLatency->setText(QString::number(metrics.minLatencyUs, 'f', 1) + " μs");
        page.lblMaxLatency->setText(QString::number(metrics.maxLatencyUs, 'f', 1) + " μs");
    } else {
        page.lblAvgLatency->setText("-- μs");
        page.lblMinLatency->setText("-- μs");
        page.lblMaxLatency->setText("-- μs");
    }
    
    // 更新批量传输指标
    page.lblBatchCount->setText(QString::number(metrics.batchCount) + " 次");
    page.lblAvgBatchSize->setText(QString::number(metrics.avgBatchSize, 'f', 1) + " 帧/批");
    page.lblBatchEfficiency->setText(QString::number(metrics.batchEfficiency, 'f', 1) + "%");
    
    // 更新压缩指标
    page.lblCompressCount->setText(QString::number(metrics.compressCount) + " 次");
    page.lblOriginalSize->setText(formatBytes(metrics.originalBytes));
    page.lblCompressedSize->setText(formatBytes(metrics.compressedBytes));
    page.lblCompressionRatio->setText(QString::number(metrics.compressionRatio, 'f', 1) + "%");
    
    // 更新错误指标
    page.lblFramesError->setText(QString::number(metrics.framesError) + " 帧");
    page.lblFramesDropped->setText(QString::number(metrics.framesDropped) + " 帧");
    page.lblErrorRate->setText(QString::number(metrics.errorRate, 'f', 2) + "%");
    
    // 更新运行状态
    page.lblUptime->setText(formatUptime(metrics.uptimeMs));
    page.lblCurrentMode->setText("批量模式");  // 简化版，实际应该从handler获取

    if (page.connection) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (page.lastFwRequestMs == 0 || now - page.lastFwRequestMs >= 1000) {
            page.connection->requestFirmwareStatistics();
            page.lastFwRequestMs = now;
        }

        auto fw = page.connection->firmwareStatistics();
        page.lblFwCan1Rx->setText(QString::number(fw.can1Rx));
        page.lblFwCan1Tx->setText(QString::number(fw.can1Tx));
        page.lblFwCan1Err->setText(QString::number(fw.can1Err));
        page.lblFwCan2Rx->setText(QString::number(fw.can2Rx));
        page.lblFwCan2Tx->setText(QString::number(fw.can2Tx));
        page.lblFwCan2Err->setText(QString::number(fw.can2Err));
        page.lblFwTotalRx->setText(QString::number(fw.totalRx));
        page.lblFwTotalBatches->setText(QString::number(fw.totalBatches));
        if (fw.updatedAtMs > 0) {
            page.lblFwUpdatedAt->setText(QDateTime::fromMSecsSinceEpoch(fw.updatedAtMs).toString("HH:mm:ss"));
        } else {
            page.lblFwUpdatedAt->setText("--");
        }
    } else {
        // 设备断开或对象已销毁：清空固件统计显示，避免访问悬空指针
        page.lblFwCan1Rx->setText("--");
        page.lblFwCan1Tx->setText("--");
        page.lblFwCan1Err->setText("--");
        page.lblFwCan2Rx->setText("--");
        page.lblFwCan2Tx->setText("--");
        page.lblFwCan2Err->setText("--");
        page.lblFwTotalRx->setText("--");
        page.lblFwTotalBatches->setText("--");
        page.lblFwUpdatedAt->setText("--");
    }
}

void StatisticsDialog::onResetClicked()
{
    // 获取当前标签页索引
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex < 0 || currentIndex >= m_devicePages.size()) {
        return;
    }
    
    DeviceStatsPage &page = m_devicePages[currentIndex];
    if (!page.statistics) {
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认重置",
        QString("确定要重置设备 %1 的统计数据吗？").arg(page.deviceName),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        page.statistics->reset();
        updateDeviceStatistics(page);
    }
}

void StatisticsDialog::onExportClicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出统计数据",
        QString("statistics_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "CSV文件 (*.csv)"
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法创建文件");
        return;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    // 写入CSV头
    out << "设备,指标类别,指标名称,数值,单位\n";
    
    // 导出所有设备的统计数据
    for (const auto &page : m_devicePages) {
        if (!page.statistics) {
            continue;
        }
        
        ZCAN::PerformanceMetrics metrics = page.statistics->getMetrics();
        QString deviceName = page.deviceName;
        
        // 吞吐量指标
        out << deviceName << ",吞吐量,接收帧数," << metrics.framesReceived << ",帧\n";
        out << deviceName << ",吞吐量,发送帧数," << metrics.framesSent << ",帧\n";
        out << deviceName << ",吞吐量,吞吐量(帧)," << metrics.throughputFps << ",帧/秒\n";
        out << deviceName << ",吞吐量,吞吐量(比特)," << metrics.throughputMbps << ",Mbps\n";
        
        // 延迟指标
        out << deviceName << ",延迟,平均延迟," << metrics.avgLatencyUs << ",μs\n";
        out << deviceName << ",延迟,最小延迟," << metrics.minLatencyUs << ",μs\n";
        out << deviceName << ",延迟,最大延迟," << metrics.maxLatencyUs << ",μs\n";
        
        // 批量传输指标
        out << deviceName << ",批量传输,批量次数," << metrics.batchCount << ",次\n";
        out << deviceName << ",批量传输,平均批量," << metrics.avgBatchSize << ",帧/批\n";
        out << deviceName << ",批量传输,批量效率," << metrics.batchEfficiency << ",%\n";
        
        // 压缩指标
        out << deviceName << ",压缩,压缩次数," << metrics.compressCount << ",次\n";
        out << deviceName << ",压缩,原始大小," << metrics.originalBytes << ",字节\n";
        out << deviceName << ",压缩,压缩后大小," << metrics.compressedBytes << ",字节\n";
        out << deviceName << ",压缩,压缩率," << metrics.compressionRatio << ",%\n";
        
        // 错误指标
        out << deviceName << ",错误,错误帧数," << metrics.framesError << ",帧\n";
        out << deviceName << ",错误,丢帧数," << metrics.framesDropped << ",帧\n";
        out << deviceName << ",错误,错误率," << metrics.errorRate << ",%\n";
        
        // 运行状态
        out << deviceName << ",运行状态,运行时间," << metrics.uptimeMs << ",毫秒\n";
    }
    
    file.close();
    
    QMessageBox::information(this, "成功", "统计数据已导出到:\n" + fileName);
}

QString StatisticsDialog::formatUptime(qint64 ms)
{
    int seconds = ms / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

QString StatisticsDialog::formatBytes(quint64 bytes)
{
    if (bytes < 1024) {
        return QString::number(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return QString::number(bytes / 1024.0, 'f', 2) + " KB";
    } else {
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + " MB";
    }
}
