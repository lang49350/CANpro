/**
 * @file FrameSenderWidget.cpp
 * @brief 帧发送窗口实现
 * @author CANAnalyzerPro Team
 * @date 2026-02-25
 */

#include "FrameSenderWidget.h"
#include "CustomComboBox.h"
#include "../core/FrameSenderCore.h"
#include "../core/ConnectionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QFile>
#include <QDebug>

/**
 * @brief 构造函数
 * @param parent 父窗口
 */
FrameSenderWidget::FrameSenderWidget(QWidget *parent)
    : QWidget(parent)
    , m_comboDevice(nullptr)
    , m_comboChannel(nullptr)
    , m_comboCanType(nullptr)
    , m_table(nullptr)
    , m_btnStartAll(nullptr)
    , m_btnStopAll(nullptr)
    , m_btnClear(nullptr)
    , m_btnLoadConfig(nullptr)
    , m_btnSaveConfig(nullptr)
    , m_btnAddRow(nullptr)
    , m_lblStatus(nullptr)
    , m_core(nullptr)
    , m_currentDevice("")
    , m_currentChannel(0)
{
    qDebug() << "🔧 创建FrameSenderWidget";
    
    // 创建核心逻辑对象
    m_core = new FrameSenderCore(this);
    
    setupUi();
    setupConnections();
    updateDeviceList();
}

/**
 * @brief 析构函数
 */
FrameSenderWidget::~FrameSenderWidget()
{
    qDebug() << "🗑 销毁FrameSenderWidget";
    
    // 停止所有发送并释放通道模式
    if (m_core) {
        m_core->stopAll();
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
void FrameSenderWidget::setupUi()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 1. 创建工具栏（设备和通道选择器）
    QWidget *toolbar = createToolbar();
    mainLayout->addWidget(toolbar);
    
    // 2. 创建表格
    QWidget *tableWidget = createTable();
    mainLayout->addWidget(tableWidget);
    
    // 3. 创建按钮栏
    QWidget *buttonBar = createButtonBar();
    mainLayout->addWidget(buttonBar);
    
    // 4. 创建状态栏
    QWidget *statusBar = createStatusBar();
    mainLayout->addWidget(statusBar);
    
    setLayout(mainLayout);
}

/**
 * @brief 创建工具栏（设备和通道选择器）
 * @return 工具栏Widget
 */
QWidget* FrameSenderWidget::createToolbar()
{
    QWidget *toolbar = new QWidget();
    toolbar->setMinimumHeight(45);
    toolbar->setMaximumHeight(45);
    toolbar->setStyleSheet("QWidget { background-color: #E8EAF6; }");
    
    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(15, 5, 15, 5);
    toolbarLayout->setSpacing(10);
    
    // 设备标签
    QLabel *lblDevice = new QLabel("设备:");
    lblDevice->setStyleSheet("color: black; font-size: 12px;");
    toolbarLayout->addWidget(lblDevice);
    
    // 设备选择器
    m_comboDevice = new CustomComboBox();
    m_comboDevice->setMinimumWidth(150);
    // 设备列表将在updateDeviceList()中填充
    toolbarLayout->addWidget(m_comboDevice);
    
    toolbarLayout->addSpacing(20);
    
    // 通道标签
    QLabel *lblChannel = new QLabel("通道:");
    lblChannel->setStyleSheet("color: black; font-size: 12px;");
    toolbarLayout->addWidget(lblChannel);
    
    // 通道选择器
    m_comboChannel = new CustomComboBox();
    m_comboChannel->setMinimumWidth(100);
    m_comboChannel->addItem("CAN0");
    m_comboChannel->addItem("CAN1");
    toolbarLayout->addWidget(m_comboChannel);

    QLabel *lblCanType = new QLabel("CAN类型:", toolbar);
    lblCanType->setStyleSheet("color: black; font-size: 12px;");
    toolbarLayout->addWidget(lblCanType);

    m_comboCanType = new CustomComboBox();
    m_comboCanType->addItem("CAN2.0", false);
    m_comboCanType->addItem("CANFD", true);
    m_comboCanType->setMinimumWidth(100);
    m_comboCanType->setCurrentIndex(0);
    toolbarLayout->addWidget(m_comboCanType);
    
    toolbarLayout->addStretch();
    
    // 加载配置按钮
    m_btnLoadConfig = new QPushButton("加载配置");
    m_btnLoadConfig->setMinimumWidth(90);
    m_btnLoadConfig->setMinimumHeight(30);
    m_btnLoadConfig->setStyleSheet(
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
    toolbarLayout->addWidget(m_btnLoadConfig);
    
    // 清空按钮
    m_btnClear = new QPushButton("清空");
    m_btnClear->setMinimumWidth(70);
    m_btnClear->setMinimumHeight(30);
    m_btnClear->setStyleSheet(
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
    toolbarLayout->addWidget(m_btnClear);
    
    return toolbar;
}

/**
 * @brief 创建表格
 * @return 表格Widget
 */
QWidget* FrameSenderWidget::createTable()
{
    QWidget *tableWidget = new QWidget();
    tableWidget->setStyleSheet("QWidget { background-color: white; }");
    
    QVBoxLayout *tableLayout = new QVBoxLayout(tableWidget);
    tableLayout->setContentsMargins(10, 10, 10, 10);
    tableLayout->setSpacing(5);
    
    // 创建表格
    m_table = new QTableWidget();
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels(
        QStringList() << "启用" << "ID" << "长度" << "扩展" << "数据" << "间隔(ms)" << "次数"
    );
    
    // 设置表格属性
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    
    // 设置列宽
    m_table->setColumnWidth(0, 60);   // 启用
    m_table->setColumnWidth(1, 100);  // ID
    m_table->setColumnWidth(2, 60);   // 长度
    m_table->setColumnWidth(3, 60);   // 扩展
    m_table->setColumnWidth(4, 250);  // 数据
    m_table->setColumnWidth(5, 100);  // 间隔
    m_table->setColumnWidth(6, 80);   // 次数
    
    // 设置表头样式
    m_table->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { "
        "   background-color: #E8EAF6; "
        "   color: black; "
        "   padding: 5px; "
        "   border: 1px solid #CCCCCC; "
        "   font-size: 12px; "
        "}"
    );
    
    // 设置表格样式
    m_table->setStyleSheet(
        "QTableWidget { "
        "   background-color: white; "
        "   alternate-background-color: #F5F5F5; "
        "   gridline-color: #E0E0E0; "
        "   border: 1px solid #CCCCCC; "
        "} "
        "QTableWidget::item { "
        "   padding: 5px; "
        "}"
    );
    
    tableLayout->addWidget(m_table);
    
    // 添加行按钮
    m_btnAddRow = new QPushButton("+ 添加行");
    m_btnAddRow->setMinimumWidth(100);
    m_btnAddRow->setMinimumHeight(28);
    m_btnAddRow->setStyleSheet(
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
    tableLayout->addWidget(m_btnAddRow, 0, Qt::AlignLeft);
    
    return tableWidget;
}

/**
 * @brief 创建按钮栏
 * @return 按钮栏Widget
 */
QWidget* FrameSenderWidget::createButtonBar()
{
    QWidget *buttonBar = new QWidget();
    buttonBar->setMinimumHeight(60);
    buttonBar->setMaximumHeight(60);
    buttonBar->setStyleSheet("QWidget { background-color: #ECEFF1; }");
    
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonBar);
    buttonLayout->setContentsMargins(20, 10, 20, 10);
    buttonLayout->setSpacing(10);
    
    // 启动全部按钮
    m_btnStartAll = new QPushButton("启动全部");
    m_btnStartAll->setMinimumWidth(100);
    m_btnStartAll->setMinimumHeight(35);
    m_btnStartAll->setStyleSheet(
        "QPushButton { "
        "   background-color: #E3F2FD; "
        "   color: black; "
        "   border: 1px solid #90CAF9; "
        "   padding: 8px 30px; "
        "   font-size: 12px; "
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
    buttonLayout->addWidget(m_btnStartAll);
    
    // 停止全部按钮
    m_btnStopAll = new QPushButton("停止全部");
    m_btnStopAll->setMinimumWidth(100);
    m_btnStopAll->setMinimumHeight(35);
    m_btnStopAll->setStyleSheet(
        "QPushButton { "
        "   background-color: #F5F5F5; "
        "   color: black; "
        "   border: 1px solid #CCCCCC; "
        "   padding: 8px 30px; "
        "   font-size: 12px; "
        "   border-radius: 3px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #E0E0E0; "
        "   border: 1px solid #999999; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #CCCCCC; "
        "}"
    );
    buttonLayout->addWidget(m_btnStopAll);
    
    buttonLayout->addStretch();
    
    // 保存配置按钮
    m_btnSaveConfig = new QPushButton("保存配置");
    m_btnSaveConfig->setMinimumWidth(100);
    m_btnSaveConfig->setMinimumHeight(35);
    m_btnSaveConfig->setStyleSheet(
        "QPushButton { "
        "   background-color: #F5F5F5; "
        "   color: black; "
        "   border: 1px solid #CCCCCC; "
        "   padding: 8px 30px; "
        "   font-size: 12px; "
        "   border-radius: 3px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #E0E0E0; "
        "   border: 1px solid #999999; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #CCCCCC; "
        "}"
    );
    buttonLayout->addWidget(m_btnSaveConfig);
    
    return buttonBar;
}

/**
 * @brief 创建状态栏
 * @return 状态栏Widget
 */
QWidget* FrameSenderWidget::createStatusBar()
{
    QWidget *statusBar = new QWidget();
    statusBar->setMinimumHeight(30);
    statusBar->setMaximumHeight(30);
    statusBar->setStyleSheet("QWidget { background-color: #ECEFF1; }");
    
    QHBoxLayout *statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(10, 0, 10, 0);
    
    // 状态标签
    m_lblStatus = new QLabel("状态: 就绪 | 已发送: 0 帧");
    m_lblStatus->setStyleSheet("color: black; font-size: 12px;");
    statusLayout->addWidget(m_lblStatus);
    
    statusLayout->addStretch();
    
    return statusBar;
}

/**
 * @brief 设置信号槽连接
 */
void FrameSenderWidget::setupConnections()
{
    // 连接添加行按钮
    connect(m_btnAddRow, &QPushButton::clicked, this, &FrameSenderWidget::onAddRow);
    
    // 连接其他按钮
    connect(m_btnStartAll, &QPushButton::clicked, this, &FrameSenderWidget::onStartAll);
    connect(m_btnStopAll, &QPushButton::clicked, this, &FrameSenderWidget::onStopAll);
    connect(m_btnClear, &QPushButton::clicked, this, &FrameSenderWidget::onClearAll);
    connect(m_btnLoadConfig, &QPushButton::clicked, this, &FrameSenderWidget::onLoadConfig);
    connect(m_btnSaveConfig, &QPushButton::clicked, this, &FrameSenderWidget::onSaveConfig);
    
    // 连接设备选择器
    connect(m_comboDevice, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
        if (index >= 0) {
            QString device = m_comboDevice->itemData(index).toString();
            setDevice(device);
        }
    });
    
    // 连接通道选择器
    connect(m_comboChannel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
        setChannel(index);
    });
    
    // 连接Core信号
    connect(m_core, &FrameSenderCore::statisticsUpdated,
            this, &FrameSenderWidget::onStatisticsUpdated);
    connect(m_core, &FrameSenderCore::errorOccurred,
            [this](const QString &message) {
        QMessageBox::warning(this, "错误", message);
    });
    
    // 连接ConnectionManager信号（监听设备连接/断开）
    connect(ConnectionManager::instance(), &ConnectionManager::deviceConnected,
            this, [this](const QString &) { updateDeviceList(); });
    connect(ConnectionManager::instance(), &ConnectionManager::deviceDisconnected,
            this, [this](const QString &) { updateDeviceList(); });
}

/**
 * @brief 设置要使用的设备
 * @param device 设备串口名称
 */
void FrameSenderWidget::setDevice(const QString &device)
{
    m_currentDevice = device;
    if (m_core) {
        m_core->setDevice(device);
    }
    qDebug() << "📡 设置设备:" << device;
}

/**
 * @brief 设置要使用的通道
 * @param channel 通道号
 */
void FrameSenderWidget::setChannel(int channel)
{
    m_currentChannel = channel;
    if (m_core) {
        m_core->setChannel(channel);
    }
    qDebug() << "📡 设置通道:" << channel;
}

/**
 * @brief 更新设备列表
 */
void FrameSenderWidget::updateDeviceList()
{
    if (!m_comboDevice) return;
    
    // 保存当前选择
    QString currentDevice = m_currentDevice;
    
    // 清空列表
    m_comboDevice->clear();
    
    // 获取已连接的设备列表
    QStringList devices = ConnectionManager::instance()->connectedDevices();
    
    if (devices.isEmpty()) {
        m_comboDevice->addItem("无可用设备", "");
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
    
    qDebug() << "🔄 更新设备列表，设备数:" << devices.size();
}

/**
 * @brief 加载配置文件
 * @param filename 配置文件路径
 */
void FrameSenderWidget::loadConfig(const QString &filename)
{
    qDebug() << "📁 加载配置:" << filename;
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "错误", "无法打开文件: " + filename);
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        QMessageBox::critical(this, "错误", "配置文件格式错误");
        return;
    }
    
    QJsonObject root = doc.object();
    
    // 读取设备和通道
    if (root.contains("device")) {
        QString device = root["device"].toString();
        int index = m_comboDevice->findData(device);
        if (index >= 0) {
            m_comboDevice->setCurrentIndex(index);
        }
    }
    
    if (root.contains("channel")) {
        int channel = root["channel"].toInt();
        m_comboChannel->setCurrentIndex(channel);
    }
    
    // 清空现有配置
    m_table->setRowCount(0);
    
    // 读取配置列表
    if (root.contains("configs") && root["configs"].isArray()) {
        QJsonArray configs = root["configs"].toArray();
        
        for (const QJsonValue &value : configs) {
            if (!value.isObject()) continue;
            
            QJsonObject config = value.toObject();
            
            // 添加新行
            int row = m_table->rowCount();
            m_table->insertRow(row);
            
            // 设置Enable
            QCheckBox *chkEnable = new QCheckBox();
            chkEnable->setChecked(config["enabled"].toBool(true));
            chkEnable->setStyleSheet("QCheckBox { margin-left: 20px; }");
            m_table->setCellWidget(row, 0, chkEnable);
            
            // 设置ID
            QLineEdit *editId = new QLineEdit();
            editId->setText(QString("0x%1").arg(config["id"].toInt(0x100), 0, 16));
            editId->setStyleSheet("QLineEdit { border: 1px solid #CCCCCC; padding: 3px; }");
            m_table->setCellWidget(row, 1, editId);
            
            // 设置Len
            QSpinBox *spinLen = new QSpinBox();
            spinLen->setRange(0, 64); // CAN-FD 允许 0..64（实际需匹配 DLC）
            spinLen->setValue(config["length"].toInt(8));
            spinLen->setStyleSheet("QSpinBox { border: 1px solid #CCCCCC; padding: 3px; }");
            m_table->setCellWidget(row, 2, spinLen);
            
            // 设置Ext
            QCheckBox *chkExt = new QCheckBox();
            chkExt->setChecked(config["isExtended"].toBool(false));
            chkExt->setStyleSheet("QCheckBox { margin-left: 20px; }");
            m_table->setCellWidget(row, 3, chkExt);
            
            // 设置Data
            QLineEdit *editData = new QLineEdit();
            editData->setText(config["data"].toString("00 00 00 00 00 00 00 00"));
            editData->setStyleSheet("QLineEdit { border: 1px solid #CCCCCC; padding: 3px; }");
            m_table->setCellWidget(row, 4, editData);
            
            // 设置间隔
            QSpinBox *spinInterval = new QSpinBox();
            spinInterval->setRange(0, 10000);
            spinInterval->setValue(config["interval"].toInt(100));
            spinInterval->setSuffix(" ms");
            spinInterval->setStyleSheet("QSpinBox { border: 1px solid #CCCCCC; padding: 3px; }");
            m_table->setCellWidget(row, 5, spinInterval);
            
            // 设置次数
            QSpinBox *spinCount = new QSpinBox();
            spinCount->setRange(-1, 999999);
            spinCount->setValue(config["maxCount"].toInt(-1));
            spinCount->setSpecialValueText("∞");
            spinCount->setStyleSheet("QSpinBox { border: 1px solid #CCCCCC; padding: 3px; }");
            m_table->setCellWidget(row, 6, spinCount);
        }
    }
    
    qDebug() << "✅ 配置加载成功";
}

/**
 * @brief 保存配置文件
 * @param filename 配置文件路径
 */
void FrameSenderWidget::saveConfig(const QString &filename)
{
    qDebug() << "📁 保存配置:" << filename;
    
    QJsonObject root;
    
    // 保存设备和通道
    root["device"] = m_currentDevice;
    root["channel"] = m_currentChannel;
    
    // 保存配置列表
    QJsonArray configs;
    
    for (int row = 0; row < m_table->rowCount(); row++) {
        QJsonObject config;
        
        // 读取Enable
        QCheckBox *chkEnable = qobject_cast<QCheckBox*>(m_table->cellWidget(row, 0));
        if (chkEnable) {
            config["enabled"] = chkEnable->isChecked();
        }
        
        // 读取ID
        QLineEdit *editId = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 1));
        if (editId) {
            QString idStr = editId->text().trimmed();
            bool ok;
            quint32 id;
            if (idStr.startsWith("0x", Qt::CaseInsensitive)) {
                id = idStr.mid(2).toUInt(&ok, 16);
            } else {
                id = idStr.toUInt(&ok, 16);
            }
            if (ok) {
                config["id"] = static_cast<int>(id);
            }
        }
        
        // 读取Len
        QSpinBox *spinLen = qobject_cast<QSpinBox*>(m_table->cellWidget(row, 2));
        if (spinLen) {
            config["length"] = spinLen->value();
        }
        
        // 读取Ext
        QCheckBox *chkExt = qobject_cast<QCheckBox*>(m_table->cellWidget(row, 3));
        if (chkExt) {
            config["isExtended"] = chkExt->isChecked();
        }
        
        // 读取Data
        QLineEdit *editData = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 4));
        if (editData) {
            config["data"] = editData->text();
        }
        
        // 读取间隔
        QSpinBox *spinInterval = qobject_cast<QSpinBox*>(m_table->cellWidget(row, 5));
        if (spinInterval) {
            config["interval"] = spinInterval->value();
        }
        
        // 读取次数
        QSpinBox *spinCount = qobject_cast<QSpinBox*>(m_table->cellWidget(row, 6));
        if (spinCount) {
            config["maxCount"] = spinCount->value();
        }
        
        configs.append(config);
    }
    
    root["configs"] = configs;
    
    // 写入文件
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "错误", "无法写入文件: " + filename);
        return;
    }
    
    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    qDebug() << "✅ 配置保存成功";
}

/**
 * @brief 启动所有已启用的发送配置
 */
void FrameSenderWidget::onStartAll()
{
    qDebug() << "▶️ 启动所有发送";

    const bool isFDWanted = (m_comboCanType ? m_comboCanType->currentData().toBool() : false);
    
    // 清空现有配置
    m_core->clearConfigs();
    
    // 从表格读取配置
    for (int row = 0; row < m_table->rowCount(); row++) {
        FrameSendConfig config;
        
        // 读取Enable
        QCheckBox *chkEnable = qobject_cast<QCheckBox*>(m_table->cellWidget(row, 0));
        if (!chkEnable) continue;
        config.enabled = chkEnable->isChecked();
        
        // 读取ID
        QLineEdit *editId = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 1));
        if (!editId) continue;
        QString idStr = editId->text().trimmed();
        bool ok;
        if (idStr.startsWith("0x", Qt::CaseInsensitive)) {
            config.id = idStr.mid(2).toUInt(&ok, 16);
        } else {
            config.id = idStr.toUInt(&ok, 16);
        }
        if (!ok) {
            QMessageBox::warning(this, "错误", QString("第%1行ID格式错误").arg(row + 1));
            return;
        }
        
        // 读取Len
        QSpinBox *spinLen = qobject_cast<QSpinBox*>(m_table->cellWidget(row, 2));
        if (!spinLen) continue;
        config.length = spinLen->value();
        
        // 读取Ext
        QCheckBox *chkExt = qobject_cast<QCheckBox*>(m_table->cellWidget(row, 3));
        if (!chkExt) continue;
        config.isExtended = chkExt->isChecked();
        
        // 读取Data
        QLineEdit *editData = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 4));
        if (!editData) continue;
        QString dataStr = editData->text().trimmed();
        QStringList dataBytes = dataStr.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        config.data.clear();
        for (const QString &byteStr : dataBytes) {
            quint8 byte = byteStr.toUInt(&ok, 16);
            if (!ok) {
                QMessageBox::warning(this, "错误", QString("第%1行数据格式错误").arg(row + 1));
                return;
            }
            config.data.append(byte);
        }

        // DLC/长度校验：避免在经典模式下误发 FD，或发送非法 DLC
        if (!isFDWanted) {
            if (config.length > 8) {
                QMessageBox::warning(
                    this,
                    "长度不合法",
                    QStringLiteral("CAN2.0 模式下，长度必须 <= 8（第%1行长度=%2）")
                        .arg(row + 1)
                        .arg(config.length));
                return;
            }
        } else {
            // CAN-FD 允许长度：0..8, 12,16,20,24,32,48,64（实际字节数）
            const bool isValidFdLen = (config.length <= 8) ||
                (config.length == 12) || (config.length == 16) || (config.length == 20) || (config.length == 24) ||
                (config.length == 32) || (config.length == 48) || (config.length == 64);
            if (!isValidFdLen) {
                QMessageBox::warning(
                    this,
                    "长度不合法",
                    QStringLiteral("CAN-FD 模式下，长度必须为 0..8 或 12/16/20/24/32/48/64（第%1行长度=%2）")
                        .arg(row + 1)
                        .arg(config.length));
                return;
            }
        }

        // 确保 data 数组长度与 length 匹配，避免越界或发送不足
        if (config.data.size() < config.length) {
            const int oldSize = config.data.size();
            config.data.resize(config.length);
            // 只对补齐的部分补 0，保留已有数据字节
            for (int i = oldSize; i < config.length; ++i) {
                config.data[i] = 0;
            }
        } else if (config.data.size() > config.length) {
            config.data = config.data.left(config.length);
        }
        
        // 读取间隔
        QSpinBox *spinInterval = qobject_cast<QSpinBox*>(m_table->cellWidget(row, 5));
        if (!spinInterval) continue;
        config.interval = spinInterval->value();
        
        // 读取次数
        QSpinBox *spinCount = qobject_cast<QSpinBox*>(m_table->cellWidget(row, 6));
        if (!spinCount) continue;
        config.maxCount = spinCount->value();
        
        // 添加配置
        m_core->addConfig(config);
    }
    
    // 🚀 使用 ConnectionManager 请求 NORMAL 模式（支持收发）
    ConnectionManager *mgr = ConnectionManager::instance();
    
    // 🛑 检查冲突
    QString reason;
    bool conflict = false;
    if (m_currentChannel == -1) {
        if (!mgr->checkModeConflict(m_currentDevice, 0, ConnectionManager::MODE_NORMAL, reason) ||
            !mgr->checkModeConflict(m_currentDevice, 1, ConnectionManager::MODE_NORMAL, reason)) {
            conflict = true;
        }
    } else {
        if (!mgr->checkModeConflict(m_currentDevice, m_currentChannel, ConnectionManager::MODE_NORMAL, reason)) {
            conflict = true;
        }
    }

    if (conflict) {
        QMessageBox::warning(this, "模式冲突", reason + "\n请先关闭相关的监听视图。");
        return;
    }

    if (m_currentChannel == -1) {
        mgr->requestChannelMode(m_currentDevice, 0, ConnectionManager::MODE_NORMAL, this, isFDWanted);
        mgr->requestChannelMode(m_currentDevice, 1, ConnectionManager::MODE_NORMAL, this, isFDWanted);
    } else {
        mgr->requestChannelMode(m_currentDevice, m_currentChannel, ConnectionManager::MODE_NORMAL, this, isFDWanted);
    }
    
    // 启动发送
    m_core->startAll();
    
    // 更新按钮状态
    m_btnStartAll->setEnabled(false);
    m_btnStopAll->setEnabled(true);
}

/**
 * @brief 停止所有发送
 */
void FrameSenderWidget::onStopAll()
{
    qDebug() << "⏹ 停止所有发送";
    
    m_core->stopAll();
    
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
    
    // 更新按钮状态
    m_btnStartAll->setEnabled(true);
    m_btnStopAll->setEnabled(false);
}

/**
 * @brief 清空所有配置
 */
void FrameSenderWidget::onClearAll()
{
    qDebug() << "🗑 清空所有配置";
    
    // 如果正在发送，先停止
    if (m_core->isRunning()) {
        m_core->stopAll();
    }
    
    // 清空表格
    m_table->setRowCount(0);
    
    // 清空Core配置
    m_core->clearConfigs();
    
    // 更新状态
    m_lblStatus->setText("状态: 就绪 | 已发送: 0 帧");
}

/**
 * @brief 加载配置按钮点击
 */
void FrameSenderWidget::onLoadConfig()
{
    QString filename = QFileDialog::getOpenFileName(
        this,
        "加载发送配置",
        "",
        "JSON文件 (*.json);;所有文件 (*.*)"
    );
    
    if (!filename.isEmpty()) {
        loadConfig(filename);
    }
}

/**
 * @brief 保存配置按钮点击
 */
void FrameSenderWidget::onSaveConfig()
{
    QString filename = QFileDialog::getSaveFileName(
        this,
        "保存发送配置",
        "",
        "JSON文件 (*.json);;所有文件 (*.*)"
    );
    
    if (!filename.isEmpty()) {
        saveConfig(filename);
    }
}

/**
 * @brief 添加新行
 */
void FrameSenderWidget::onAddRow()
{
    qDebug() << "➕ 添加新行";
    
    // 添加新行
    int row = m_table->rowCount();
    m_table->insertRow(row);
    
    // 列0: Enable列（勾选框）
    QCheckBox *chkEnable = new QCheckBox();
    chkEnable->setChecked(true);
    chkEnable->setStyleSheet("QCheckBox { margin-left: 20px; }");
    m_table->setCellWidget(row, 0, chkEnable);
    
    // 列1: ID列（文本输入）
    QLineEdit *editId = new QLineEdit();
    editId->setText("0x100");
    editId->setPlaceholderText("0x100");
    editId->setStyleSheet("QLineEdit { border: 1px solid #CCCCCC; padding: 3px; }");
    m_table->setCellWidget(row, 1, editId);
    
    // 列2: Len列（数字输入）
    QSpinBox *spinLen = new QSpinBox();
    spinLen->setRange(0, 64); // CAN-FD 允许 0..64
    spinLen->setValue(8);
    spinLen->setStyleSheet("QSpinBox { border: 1px solid #CCCCCC; padding: 3px; }");
    m_table->setCellWidget(row, 2, spinLen);
    
    // 列3: Ext列（勾选框）
    QCheckBox *chkExt = new QCheckBox();
    chkExt->setChecked(false);
    chkExt->setStyleSheet("QCheckBox { margin-left: 20px; }");
    m_table->setCellWidget(row, 3, chkExt);
    
    // 列4: Data列（文本输入）
    QLineEdit *editData = new QLineEdit();
    editData->setText("00 00 00 00 00 00 00 00");
    editData->setPlaceholderText("00 00 00 00 00 00 00 00");
    editData->setStyleSheet("QLineEdit { border: 1px solid #CCCCCC; padding: 3px; }");
    m_table->setCellWidget(row, 4, editData);
    
    // 列5: 间隔列（数字输入）
    QSpinBox *spinInterval = new QSpinBox();
    spinInterval->setRange(0, 10000);
    spinInterval->setValue(100);
    spinInterval->setSuffix(" ms");
    spinInterval->setStyleSheet("QSpinBox { border: 1px solid #CCCCCC; padding: 3px; }");
    m_table->setCellWidget(row, 5, spinInterval);
    
    // 列6: 次数列（数字输入，-1表示无限）
    QSpinBox *spinCount = new QSpinBox();
    spinCount->setRange(-1, 999999);
    spinCount->setValue(-1);
    spinCount->setSpecialValueText("∞");
    spinCount->setStyleSheet("QSpinBox { border: 1px solid #CCCCCC; padding: 3px; }");
    m_table->setCellWidget(row, 6, spinCount);
    
    qDebug() << "✅ 添加新行成功，当前行数:" << m_table->rowCount();
}

/**
 * @brief 表格单元格内容改变
 * @param row 行号
 * @param col 列号
 */
void FrameSenderWidget::onCellChanged(int row, int col)
{
    qDebug() << "📝 单元格改变: 行" << row << "列" << col;
    // TODO: 实现单元格改变处理
}

/**
 * @brief 更新统计信息
 * @param totalSent 总发送帧数
 */
void FrameSenderWidget::onStatisticsUpdated(int totalSent)
{
    if (m_lblStatus) {
        m_lblStatus->setText(QString("已发送: %1 帧").arg(totalSent));
    }
}

/**
 * @brief 更新状态栏
 */
void FrameSenderWidget::updateStatusBar()
{
    // TODO: 实现状态栏更新
}