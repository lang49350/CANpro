/**
 * @file DeviceManagerDialog.cpp
 * @brief 设备管理对话框实现
 * @author CANAnalyzerPro Team
 * @date 2024-02-08
 */

#include "DeviceManagerDialog.h"
#include "CustomComboBox.h"
#include "MainWindow.h"
#include "../core/ConnectionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QGroupBox>
#include <QFormLayout>
#include <QMouseEvent>
#include <QDebug>

DeviceManagerDialog::DeviceManagerDialog(QWidget *parent)
    : QDialog(parent)
    , m_dragging(false)
    , m_isMaximized(false)
{
    // 设置窗口标志：去掉系统标题栏，添加边框样式
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground, false);
    
    // 使用对象名称选择器来应用样式
    setObjectName("DeviceManagerDialog");
    setStyleSheet(
        "#DeviceManagerDialog { "
        "   background-color: white; "
        "   border: 2px solid #607D8B; "
        "   border-radius: 5px; "
        "}"
    );
    
    setupUi();
    setupConnections();
    
    // 初始化设备列表
    updateDeviceList();
    
    // 启动状态更新定时器（每500ms更新一次）
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &DeviceManagerDialog::updateDeviceStatus);
    m_updateTimer->start(500);
    
    qDebug() << "✅ 设备管理对话框初始化完成";
}

DeviceManagerDialog::~DeviceManagerDialog()
{
    m_updateTimer->stop();
}

void DeviceManagerDialog::setupUi()
{
    setMinimumSize(900, 600);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // ==================== 自定义标题栏 ====================
    m_titleBar = createTitleBar();
    mainLayout->addWidget(m_titleBar);
    
    // ==================== 内容区域 ====================
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(10);
    contentLayout->setContentsMargins(10, 10, 10, 10);
    
    // ==================== 顶部工具栏 ====================
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    
    // 工具按钮样式
    QString toolButtonStyle = 
        "QPushButton { "
        "   background-color: white; "
        "   border: 1px solid #CCCCCC; "
        "   padding: 5px 15px; "
        "   border-radius: 3px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #E3F2FD; "
        "   border: 1px solid #90CAF9; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #BBDEFB; "
        "} "
        "QPushButton:disabled { "
        "   background-color: #F5F5F5; "
        "   color: #BDBDBD; "
        "   border: 1px solid #E0E0E0; "
        "}";
    
    m_btnRefresh = new QPushButton("🔄 刷新设备");
    m_btnRefresh->setStyleSheet(toolButtonStyle);
    
    m_btnConnect = new QPushButton("✅ 连接");
    m_btnConnect->setStyleSheet(toolButtonStyle);
    m_btnConnect->setEnabled(false);
    
    m_btnDisconnect = new QPushButton("❌ 断开");
    m_btnDisconnect->setStyleSheet(toolButtonStyle);
    m_btnDisconnect->setEnabled(false);
    
    m_btnConfig = new QPushButton("⚙ 配置");
    m_btnConfig->setStyleSheet(toolButtonStyle);
    m_btnConfig->setEnabled(false);
    
    toolbarLayout->addWidget(m_btnRefresh);
    toolbarLayout->addWidget(m_btnConnect);
    toolbarLayout->addWidget(m_btnDisconnect);
    toolbarLayout->addWidget(m_btnConfig);
    toolbarLayout->addStretch();
    
    contentLayout->addLayout(toolbarLayout);
    
    // ==================== 设备列表表格 ====================
    m_deviceTable = new QTableWidget(0, 8);
    m_deviceTable->setHorizontalHeaderLabels(QStringList() 
        << "串口" << "描述" << "状态" << "波特率" 
        << "通道数" << "接收帧数" << "发送帧数" << "错误数");
    
    // 设置表格样式
    m_deviceTable->setStyleSheet(
        "QTableWidget { "
        "   background-color: white; "
        "   gridline-color: #E0E0E0; "
        "   selection-background-color: #BBDEFB; "
        "} "
        "QHeaderView::section { "
        "   background-color: #CFD8DC; "
        "   padding: 5px; "
        "   border: 1px solid #B0BEC5; "
        "   font-weight: bold; "
        "}"
    );
    
    m_deviceTable->setAlternatingRowColors(true);
    m_deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_deviceTable->horizontalHeader()->setStretchLastSection(true);
    
    // 设置列宽
    m_deviceTable->setColumnWidth(0, 80);   // 串口
    m_deviceTable->setColumnWidth(1, 200);  // 描述
    m_deviceTable->setColumnWidth(2, 80);   // 状态
    m_deviceTable->setColumnWidth(3, 100);  // 波特率
    m_deviceTable->setColumnWidth(4, 80);   // 通道数
    m_deviceTable->setColumnWidth(5, 100);  // 接收帧数
    m_deviceTable->setColumnWidth(6, 100);  // 发送帧数
    
    contentLayout->addWidget(m_deviceTable);
    
    // ==================== 配置区域 ====================
    QGroupBox *configGroup = new QGroupBox("设备配置");
    configGroup->setMaximumHeight(120);  // 限制配置区域高度
    QFormLayout *configLayout = new QFormLayout(configGroup);
    
    m_comboBaudRate = new CustomComboBox();
    m_comboBaudRate->addItems(QStringList() 
        << "9600" << "19200" << "38400" << "57600" 
        << "115200" << "230400" << "460800" << "921600" 
        << "1000000" << "2000000");
    m_comboBaudRate->setCurrentText("2000000");
    
    m_spinChannels = new QSpinBox();
    m_spinChannels->setRange(1, 4);
    m_spinChannels->setValue(2);
    m_spinChannels->setSuffix(" 通道");
    
    configLayout->addRow("波特率:", m_comboBaudRate);
    configLayout->addRow("通道数:", m_spinChannels);
    
    contentLayout->addWidget(configGroup);
    
    // ==================== 状态栏 ====================
    m_lblStatus = new QLabel("就绪");
    m_lblStatus->setStyleSheet("color: green; font-weight: bold;");
    contentLayout->addWidget(m_lblStatus);
    
    // ==================== 底部按钮 ====================
    QWidget *bottomBar = new QWidget();
    bottomBar->setStyleSheet("QWidget { background-color: #ECEFF1; }");
    bottomBar->setMinimumHeight(60);
    bottomBar->setMaximumHeight(60);
    
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(20, 10, 20, 10);
    bottomLayout->addStretch();
    
    m_btnClose = new QPushButton("确定");
    m_btnClose->setMinimumWidth(100);
    m_btnClose->setMinimumHeight(35);  // 设置按钮最小高度
    m_btnClose->setStyleSheet(
        "QPushButton { "
        "   background-color: #E3F2FD; "  // 浅蓝色背景（类似配置按钮）
        "   color: black; "
        "   border: 1px solid #90CAF9; "
        "   padding: 8px 30px; "
        "   font-size: 12px; "
        "   border-radius: 3px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #BBDEFB; "  // 悬停时更深的蓝色
        "   border: 1px solid #64B5F6; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #90CAF9; "  // 按下时更深
        "}"
    );
    bottomLayout->addWidget(m_btnClose);
    
    contentLayout->addWidget(bottomBar);
    
    // 添加内容区域到主布局
    mainLayout->addWidget(contentWidget);
}

void DeviceManagerDialog::setupConnections()
{
    // 按钮信号
    connect(m_btnRefresh, &QPushButton::clicked, this, &DeviceManagerDialog::onRefreshClicked);
    connect(m_btnConnect, &QPushButton::clicked, this, &DeviceManagerDialog::onConnectClicked);
    connect(m_btnDisconnect, &QPushButton::clicked, this, &DeviceManagerDialog::onDisconnectClicked);
    connect(m_btnConfig, &QPushButton::clicked, this, &DeviceManagerDialog::onConfigClicked);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_btnMaximize, &QPushButton::clicked, this, &DeviceManagerDialog::onMaximizeClicked);
    
    // 表格选择改变
    connect(m_deviceTable, &QTableWidget::itemSelectionChanged, 
            this, &DeviceManagerDialog::onDeviceSelectionChanged);
    
    // 连接ConnectionManager的信号
    ConnectionManager *connMgr = ConnectionManager::instance();
    
    connect(connMgr, &ConnectionManager::deviceConnected,
            this, [this](const QString &port) {
        qDebug() << "✅ 设备连接成功:" << port;
        m_lblStatus->setText("设备连接成功: " + port);
        m_lblStatus->setStyleSheet("color: green; font-weight: bold;");
        updateDeviceStatus();
        
        // 立即更新按钮状态
        QList<QTableWidgetItem*> selectedItems = m_deviceTable->selectedItems();
        if (!selectedItems.isEmpty()) {
            int row = selectedItems.first()->row();
            QString selectedPort = m_deviceTable->item(row, 0)->text();
            if (selectedPort == port) {
                // 如果当前选中的就是刚连接的设备，更新按钮状态
                m_btnConnect->setEnabled(false);
                m_btnDisconnect->setEnabled(true);
            }
        }
    });
    
    connect(connMgr, &ConnectionManager::deviceDisconnected,
            this, [this](const QString &port) {
        qDebug() << "🔌 设备断开:" << port;
        m_lblStatus->setText("设备断开: " + port);
        m_lblStatus->setStyleSheet("color: orange; font-weight: bold;");
        updateDeviceStatus();
        
        // 立即更新按钮状态
        QList<QTableWidgetItem*> selectedItems = m_deviceTable->selectedItems();
        if (!selectedItems.isEmpty()) {
            int row = selectedItems.first()->row();
            QString selectedPort = m_deviceTable->item(row, 0)->text();
            if (selectedPort == port) {
                // 如果当前选中的就是刚断开的设备，更新按钮状态
                m_btnConnect->setEnabled(true);
                m_btnDisconnect->setEnabled(false);
            }
        }
    });
    
    connect(connMgr, &ConnectionManager::errorOccurred,
            this, [this](const QString &port, const QString &error) {
        qWarning() << "❌ 设备错误:" << port << error;
        m_lblStatus->setText("错误: " + error);
        m_lblStatus->setStyleSheet("color: red; font-weight: bold;");
    });
}

void DeviceManagerDialog::onRefreshClicked()
{
    qDebug() << "🔄 刷新设备列表";
    updateDeviceList();
    m_lblStatus->setText("设备列表已刷新");
    m_lblStatus->setStyleSheet("color: blue; font-weight: bold;");
}

void DeviceManagerDialog::onConnectClicked()
{
    QList<QTableWidgetItem*> selectedItems = m_deviceTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择要连接的设备");
        return;
    }
    
    int row = selectedItems.first()->row();
    QString port = m_deviceTable->item(row, 0)->text();
    QString status = m_deviceTable->item(row, 2)->text();
    
    // 检查是否已连接
    if (status == "已连接") {
        QMessageBox::information(this, "提示", "设备已经连接");
        return;
    }
    
    // 获取配置参数
    int baudRate = m_comboBaudRate->currentText().toInt();
    
    // 连接设备
    ConnectionManager *connMgr = ConnectionManager::instance();
    if (connMgr->connectDevice(port, baudRate)) {
        qDebug() << "✅ 连接设备成功:" << port << "波特率:" << baudRate;
    } else {
        QMessageBox::critical(this, "连接失败", "无法打开串口: " + port);
    }
}

void DeviceManagerDialog::onDisconnectClicked()
{
    QList<QTableWidgetItem*> selectedItems = m_deviceTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择要断开的设备");
        return;
    }
    
    int row = selectedItems.first()->row();
    QString port = m_deviceTable->item(row, 0)->text();
    QString status = m_deviceTable->item(row, 2)->text();
    
    // 检查是否已连接
    if (status != "已连接") {
        QMessageBox::information(this, "提示", "设备未连接");
        return;
    }
    
    // 断开设备
    ConnectionManager *connMgr = ConnectionManager::instance();
    connMgr->disconnectDevice(port);
    qDebug() << "🔌 断开设备:" << port;
}

void DeviceManagerDialog::onConfigClicked()
{
    QList<QTableWidgetItem*> selectedItems = m_deviceTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择要配置的设备");
        return;
    }
    
    int row = selectedItems.first()->row();
    QString port = m_deviceTable->item(row, 0)->text();
    
    // 打开协议配置对话框
    QMessageBox::information(this, "设备配置", 
        QString("设备: %1\n\n请使用主窗口的\"协议模式设置\"进行配置").arg(port));
}

void DeviceManagerDialog::onDeviceSelectionChanged()
{
    bool hasSelection = !m_deviceTable->selectedItems().isEmpty();
    
    if (hasSelection) {
        int row = m_deviceTable->selectedItems().first()->row();
        QString status = m_deviceTable->item(row, 2)->text();
        
        // 根据连接状态启用/禁用按钮
        m_btnConnect->setEnabled(status != "已连接");
        m_btnDisconnect->setEnabled(status == "已连接");
        m_btnConfig->setEnabled(true);
    } else {
        m_btnConnect->setEnabled(false);
        m_btnDisconnect->setEnabled(false);
        m_btnConfig->setEnabled(false);
    }
}

void DeviceManagerDialog::updateDeviceList()
{
    // 清空表格
    m_deviceTable->setRowCount(0);
    
    // 获取所有可用串口
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    ConnectionManager *connMgr = ConnectionManager::instance();
    QStringList connectedDevices = connMgr->connectedDevices();

    // Filter: Only show "our" USB CDC ports (TinyUSB/ESP32 typical VID/PID).
    // If the platform does not report VID/PID, fall back to description/manufacturer keywords.
    auto isOurUsbCdcPort = [](const QSerialPortInfo &info) -> bool {
        const quint16 vid = info.vendorIdentifier();
        const quint16 pid = info.productIdentifier();

        // Default TinyUSB CDC IDs are commonly 0x303A:0x1001 for ESP32.
        // (If your descriptor uses different IDs, adjust here.)
        if (vid != 0 && pid != 0) {
            return (vid == 0x303A && pid == 0x1001);
        }

        const QString desc = info.description().toLower();
        const QString manuf = info.manufacturer().toLower();
        const QString sysLoc = info.systemLocation().toLower();

        const bool hasUsbSerialKeyword =
            desc.contains("usb serial") ||
            desc.contains("usb 串行") ||
            desc.contains("cdc") ||
            sysLoc.contains("cdc");

        const bool seemsEspressif =
            manuf.contains("espressif") ||
            manuf.contains("esp32") ||
            sysLoc.contains("vid_303a") ||
            sysLoc.contains("pid_1001");

        return hasUsbSerialKeyword && seemsEspressif;
    };
    
    qDebug() << "📋 可用串口数量:" << ports.count();

    int shownCount = 0;
    
    for (const QSerialPortInfo &portInfo : ports) {
        QString port = portInfo.portName();
        QString description = portInfo.description();
        bool connected = connectedDevices.contains(port);

        // Always show connected ports; for disconnected ports, show only our CDC.
        if (connected || isOurUsbCdcPort(portInfo)) {
            addDeviceToTable(port, description, connected);
            shownCount++;
        }
    }
    
    m_lblStatus->setText(QString("找到 %1 个CDC设备").arg(shownCount));
}

void DeviceManagerDialog::updateDeviceStatus()
{
    ConnectionManager *connMgr = ConnectionManager::instance();
    QStringList connectedDevices = connMgr->connectedDevices();
    
    // 更新每一行的状态
    for (int row = 0; row < m_deviceTable->rowCount(); ++row) {
        QString port = m_deviceTable->item(row, 0)->text();
        bool connected = connectedDevices.contains(port);
        
        // 更新状态列
        QTableWidgetItem *statusItem = m_deviceTable->item(row, 2);
        if (statusItem) {
            if (connected) {
                statusItem->setText("已连接");
                statusItem->setForeground(QBrush(Qt::darkGreen));
            } else {
                statusItem->setText("未连接");
                statusItem->setForeground(QBrush(Qt::gray));
            }
        }
    }
}

void DeviceManagerDialog::addDeviceToTable(const QString &port, 
                                           const QString &description, 
                                           bool connected)
{
    int row = m_deviceTable->rowCount();
    m_deviceTable->insertRow(row);
    
    // 串口
    QTableWidgetItem *portItem = new QTableWidgetItem(port);
    m_deviceTable->setItem(row, 0, portItem);
    
    // 描述
    QTableWidgetItem *descItem = new QTableWidgetItem(description);
    m_deviceTable->setItem(row, 1, descItem);
    
    // 状态
    QTableWidgetItem *statusItem = new QTableWidgetItem(connected ? "已连接" : "未连接");
    statusItem->setForeground(connected ? QBrush(Qt::darkGreen) : QBrush(Qt::gray));
    m_deviceTable->setItem(row, 2, statusItem);
    
    // 波特率
    QTableWidgetItem *baudItem = new QTableWidgetItem(connected ? "2000000" : "-");
    m_deviceTable->setItem(row, 3, baudItem);
    
    // 通道数
    QTableWidgetItem *channelItem = new QTableWidgetItem(connected ? "2" : "-");
    m_deviceTable->setItem(row, 4, channelItem);
    
    // 接收帧数
    QTableWidgetItem *rxItem = new QTableWidgetItem("0");
    m_deviceTable->setItem(row, 5, rxItem);
    
    // 发送帧数
    QTableWidgetItem *txItem = new QTableWidgetItem("0");
    m_deviceTable->setItem(row, 6, txItem);
    
    // 错误数
    QTableWidgetItem *errorItem = new QTableWidgetItem("0");
    m_deviceTable->setItem(row, 7, errorItem);
}

int DeviceManagerDialog::findDeviceRow(const QString &port)
{
    for (int row = 0; row < m_deviceTable->rowCount(); ++row) {
        if (m_deviceTable->item(row, 0)->text() == port) {
            return row;
        }
    }
    return -1;
}


// ==================== 自定义标题栏 ====================

QWidget* DeviceManagerDialog::createTitleBar()
{
    QWidget *titleBar = new QWidget();
    titleBar->setMinimumHeight(40);
    titleBar->setMaximumHeight(40);
    titleBar->setStyleSheet("QWidget { background-color: #607D8B; }");  // 蓝灰色
    
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setSpacing(0);
    titleLayout->setContentsMargins(10, 0, 0, 0);
    
    // 标题文字
    QLabel *lblTitle = new QLabel("设备管理");
    lblTitle->setStyleSheet("color: white; font-size: 14px; font-weight: bold;");
    titleLayout->addWidget(lblTitle);
    
    titleLayout->addStretch();
    
    // 按钮样式
    QString btnStyle = 
        "QPushButton { "
        "   background-color: transparent; "
        "   color: white; "
        "   border: none; "
        "   padding: 0px; "
        "   font-size: 16px; "
        "   min-width: 40px; "
        "   max-width: 40px; "
        "   min-height: 40px; "
        "   max-height: 40px; "
        "} "
        "QPushButton:hover { "
        "   background-color: rgba(255,255,255,0.2); "
        "}";
    
    QString closeBtnStyle = btnStyle + 
        "QPushButton:hover { "
        "   background-color: #E81123; "
        "}";
    
    // 最大化按钮
    m_btnMaximize = new QPushButton("□");
    m_btnMaximize->setStyleSheet(btnStyle);
    m_btnMaximize->setToolTip("最大化");
    titleLayout->addWidget(m_btnMaximize);
    
    // 关闭按钮
    QPushButton *btnClose = new QPushButton("✕");
    btnClose->setStyleSheet(closeBtnStyle);
    btnClose->setToolTip("关闭");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    titleLayout->addWidget(btnClose);
    
    return titleBar;
}

void DeviceManagerDialog::onMaximizeClicked()
{
    MainWindow *mainWin = qobject_cast<MainWindow*>(parentWidget());
    if (!mainWin) {
        qWarning() << "⚠️ 没有父窗口，无法在主窗口内最大化";
        return;
    }
    
    if (m_isMaximized) {
        // 恢复正常大小
        setGeometry(m_normalGeometry);
        m_btnMaximize->setText("□");
        m_btnMaximize->setToolTip("最大化");
        m_isMaximized = false;
        qDebug() << "📐 恢复正常大小";
    } else {
        // 保存当前几何位置
        m_normalGeometry = geometry();
        
        // 获取MDI区域的几何位置
        QMdiArea *mdiArea = mainWin->mdiArea();
        if (!mdiArea) {
            qWarning() << "⚠️ 无法获取MDI区域";
            return;
        }
        
        // 获取MDI区域在全局坐标系中的位置和大小
        QPoint mdiGlobalPos = mdiArea->mapToGlobal(QPoint(0, 0));
        QSize mdiSize = mdiArea->size();
        
        // 设置对话框在MDI区域内最大化
        setGeometry(QRect(mdiGlobalPos, mdiSize));
        m_btnMaximize->setText("❐");
        m_btnMaximize->setToolTip("还原");
        m_isMaximized = true;
        qDebug() << "📐 在MDI区域内最大化:" << QRect(mdiGlobalPos, mdiSize);
    }
}

// ==================== 窗口拖动实现 ====================

void DeviceManagerDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 只在标题栏区域允许拖动（前40像素高度）
        if (event->pos().y() <= 40) {
            m_dragging = true;
            m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
        }
    }
    QDialog::mousePressEvent(event);
}

void DeviceManagerDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        // 如果已最大化，拖动时先恢复正常大小
        if (m_isMaximized) {
            m_isMaximized = false;
            m_btnMaximize->setText("□");
            m_btnMaximize->setToolTip("最大化");
            
            // 恢复到正常大小，但位置跟随鼠标
            QPoint newPos = event->globalPosition().toPoint() - m_dragPosition;
            setGeometry(QRect(newPos, m_normalGeometry.size()));
        } else {
            move(event->globalPosition().toPoint() - m_dragPosition);
        }
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

void DeviceManagerDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
    QDialog::mouseReleaseEvent(event);
}
