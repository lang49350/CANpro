#include "ProtocolConfigDialog.h"
#include "../core/SerialConnection.h"
#include "../core/zcan/zcan_types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QSettings>

ProtocolConfigDialog::ProtocolConfigDialog(const QMap<QString, SerialConnection*> &connections, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("UCAN-FD 设备参数"));
    setMinimumSize(520, 420);
    
    for (auto it = connections.constBegin(); it != connections.constEnd(); ++it) {
        QString deviceName = it.key();
        SerialConnection *conn = it.value();
        
        if (conn) {
            DeviceConfigPage page;
            page.deviceName = deviceName;
            page.connection = conn;
            page.pageWidget = nullptr;
            m_devicePages.append(page);
            m_devicePages.last().pageWidget = createDevicePage(deviceName);
        }
    }
    
    setupUi();
    
    for (auto &page : m_devicePages) {
        loadDeviceSettings(page);
    }
}

ProtocolConfigDialog::~ProtocolConfigDialog() = default;

void ProtocolConfigDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    m_tabWidget = new QTabWidget();
    
    for (int i = 0; i < m_devicePages.size(); i++) {
        m_tabWidget->addTab(m_devicePages[i].pageWidget, m_devicePages[i].deviceName);
    }
    
    if (m_devicePages.isEmpty()) {
        QWidget *emptyPage = new QWidget();
        QVBoxLayout *emptyLayout = new QVBoxLayout(emptyPage);
        QLabel *emptyLabel = new QLabel(QStringLiteral("没有连接的设备"));
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet(QStringLiteral("color: gray; font-size: 14px;"));
        emptyLayout->addWidget(emptyLabel);
        m_tabWidget->addTab(emptyPage, QStringLiteral("无设备"));
    }
    
    mainLayout->addWidget(m_tabWidget);
    mainLayout->addStretch();
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton *btnApply = new QPushButton(QStringLiteral("应用当前设备"));
    QPushButton *btnApplyAll = new QPushButton(QStringLiteral("应用所有设备"));
    QPushButton *btnSave = new QPushButton(QStringLiteral("保存"));
    QPushButton *btnCancel = new QPushButton(QStringLiteral("取消"));
    
    connect(btnApply, &QPushButton::clicked, this, &ProtocolConfigDialog::onApplyClicked);
    connect(btnApplyAll, &QPushButton::clicked, this, [this]() {
        for (auto &page : m_devicePages) {
            applyDeviceSettings(page);
        }
        QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("所有设备的设置已应用"));
    });
    connect(btnSave, &QPushButton::clicked, this, &ProtocolConfigDialog::onSaveClicked);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    
    buttonLayout->addWidget(btnApply);
    buttonLayout->addWidget(btnApplyAll);
    buttonLayout->addWidget(btnSave);
    buttonLayout->addWidget(btnCancel);
    
    mainLayout->addLayout(buttonLayout);
}

QWidget* ProtocolConfigDialog::createDevicePage(const QString &deviceName)
{
    Q_UNUSED(deviceName);
    QWidget *pageWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(pageWidget);
    layout->setSpacing(15);
    layout->setContentsMargins(10, 10, 10, 10);
    
    DeviceConfigPage &page = m_devicePages.last();
    
    QGroupBox *modeGroup = new QGroupBox(QStringLiteral("链路协议"));
    QVBoxLayout *modeLayout = new QVBoxLayout(modeGroup);
    page.lblProtocol = new QLabel(QStringLiteral(
        "固定为 UCAN-FD：A6 + type + flags + seq + len16 + payload，批量上行使用 RX_BATCH。"));
    page.lblProtocol->setWordWrap(true);
    page.lblProtocol->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    modeLayout->addWidget(page.lblProtocol);
    layout->addWidget(modeGroup);
    
    QGroupBox *advancedGroup = new QGroupBox(QStringLiteral("高级选项"));
    QVBoxLayout *advancedLayout = new QVBoxLayout(advancedGroup);
    advancedLayout->setSpacing(10);
    
    page.chkZeroCopy = new QCheckBox(QStringLiteral("启用零拷贝优化"));
    QLabel *lblZeroCopy = new QLabel(QStringLiteral("    解析线程侧零拷贝批量解析（推荐开启）"));
    lblZeroCopy->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    advancedLayout->addWidget(page.chkZeroCopy);
    advancedLayout->addWidget(lblZeroCopy);
    advancedLayout->addSpacing(10);
    
    QHBoxLayout *batchSizeLayout = new QHBoxLayout();
    batchSizeLayout->addWidget(new QLabel(QStringLiteral("批量大小：")));
    page.comboBatchSize = new QComboBox();
    page.comboBatchSize->addItem(QStringLiteral("10 帧/批"), 10);
    page.comboBatchSize->addItem(QStringLiteral("20 帧/批"), 20);
    page.comboBatchSize->addItem(QStringLiteral("50 帧/批"), 50);
    page.comboBatchSize->addItem(QStringLiteral("100 帧/批"), 100);
    page.comboBatchSize->addItem(QStringLiteral("200 帧/批"), 200);
    page.comboBatchSize->addItem(QStringLiteral("500 帧/批"), 500);
    page.comboBatchSize->addItem(QStringLiteral("1000 帧/批"), 1000);
    page.comboBatchSize->setCurrentIndex(2);
    batchSizeLayout->addWidget(page.comboBatchSize);
    batchSizeLayout->addStretch();
    advancedLayout->addLayout(batchSizeLayout);
    
    QLabel *lblBatchSize = new QLabel(QStringLiteral("    (下发给固件的每批最大帧数，范围 1–50)"));
    lblBatchSize->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    advancedLayout->addWidget(lblBatchSize);
    advancedLayout->addSpacing(10);
    
    QHBoxLayout *statsIntervalLayout = new QHBoxLayout();
    statsIntervalLayout->addWidget(new QLabel(QStringLiteral("统计间隔：")));
    page.comboStatsInterval = new QComboBox();
    page.comboStatsInterval->addItem(QStringLiteral("50 毫秒"), 50);
    page.comboStatsInterval->addItem(QStringLiteral("100 毫秒"), 100);
    page.comboStatsInterval->addItem(QStringLiteral("200 毫秒"), 200);
    page.comboStatsInterval->addItem(QStringLiteral("500 毫秒"), 500);
    page.comboStatsInterval->addItem(QStringLiteral("1000 毫秒"), 1000);
    page.comboStatsInterval->addItem(QStringLiteral("2000 毫秒"), 2000);
    page.comboStatsInterval->addItem(QStringLiteral("5000 毫秒"), 5000);
    page.comboStatsInterval->setCurrentIndex(1);
    statsIntervalLayout->addWidget(page.comboStatsInterval);
    statsIntervalLayout->addStretch();
    advancedLayout->addLayout(statsIntervalLayout);
    
    QLabel *lblStatsInterval = new QLabel(QStringLiteral("    (范围: 50-5000)"));
    lblStatsInterval->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    advancedLayout->addWidget(lblStatsInterval);
    
    layout->addWidget(advancedGroup);
    
    QGroupBox *statusGroup = new QGroupBox(QStringLiteral("当前状态"));
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    page.lblDeviceSupport = new QLabel(QStringLiteral("设备支持：检测中..."));
    page.lblFirmwareVersion = new QLabel(QStringLiteral("固件版本：--"));
    statusLayout->addWidget(page.lblDeviceSupport);
    statusLayout->addWidget(page.lblFirmwareVersion);
    layout->addWidget(statusGroup);
    
    layout->addStretch();
    
    return pageWidget;
}

void ProtocolConfigDialog::loadDeviceSettings(DeviceConfigPage &page)
{
    if (!page.connection) {
        page.lblDeviceSupport->setText(QStringLiteral("设备支持：设备已断开"));
        page.lblFirmwareVersion->setText(QStringLiteral("固件版本：--"));
        page.chkZeroCopy->setEnabled(false);
        page.comboBatchSize->setEnabled(false);
        page.comboStatsInterval->setEnabled(false);
        return;
    }
    
    page.chkZeroCopy->setChecked(page.connection->isZeroCopyEnabled());
    page.chkZeroCopy->setEnabled(true);
    page.comboBatchSize->setEnabled(true);
    page.comboStatsInterval->setEnabled(true);
    
    if (page.connection->isUCANSupported()) {
        page.lblDeviceSupport->setText(QStringLiteral("设备支持：UCAN-FD 批量"));
    } else {
        page.lblDeviceSupport->setText(QStringLiteral("设备支持：未识别（仍按 UCAN-FD 解析）"));
    }
    
    QString firmwareVersion = page.connection->ucanFirmwareVersion();
    page.lblFirmwareVersion->setText(QStringLiteral("固件版本：%1")
        .arg(firmwareVersion.isEmpty() ? QStringLiteral("--") : firmwareVersion));
    
    QSettings settings(QStringLiteral("CANTools"), QStringLiteral("CANAnalyzerPro"));
    QString deviceKey = page.deviceName.replace(QLatin1Char(' '), QLatin1Char('_'));
    int batchSize = settings.value(QStringLiteral("UCAN/%1/batchSize").arg(deviceKey), 50).toInt();
    int statsInterval = settings.value(QStringLiteral("UCAN/%1/statsInterval").arg(deviceKey), 100).toInt();
    
    for (int i = 0; i < page.comboBatchSize->count(); i++) {
        if (page.comboBatchSize->itemData(i).toInt() == batchSize) {
            page.comboBatchSize->setCurrentIndex(i);
            break;
        }
    }
    
    for (int i = 0; i < page.comboStatsInterval->count(); i++) {
        if (page.comboStatsInterval->itemData(i).toInt() == statsInterval) {
            page.comboStatsInterval->setCurrentIndex(i);
            break;
        }
    }
}

void ProtocolConfigDialog::onApplyClicked()
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex < 0 || currentIndex >= m_devicePages.size()) {
        return;
    }
    
    applyDeviceSettings(m_devicePages[currentIndex]);
    QMessageBox::information(this, QStringLiteral("成功"),
        QStringLiteral("设备 %1 的设置已应用").arg(m_devicePages[currentIndex].deviceName));
}

void ProtocolConfigDialog::onSaveClicked()
{
    QSettings settings(QStringLiteral("CANTools"), QStringLiteral("CANAnalyzerPro"));
    
    for (auto &page : m_devicePages) {
        applyDeviceSettings(page);
        
        QString deviceKey = page.deviceName.replace(QLatin1Char(' '), QLatin1Char('_'));
        settings.setValue(QStringLiteral("UCAN/%1/mode").arg(deviceKey), QStringLiteral("UCAN-FD"));
        settings.setValue(QStringLiteral("UCAN/%1/zeroCopy").arg(deviceKey), page.chkZeroCopy->isChecked());
        settings.setValue(QStringLiteral("UCAN/%1/batchSize").arg(deviceKey), page.comboBatchSize->currentData().toInt());
        settings.setValue(QStringLiteral("UCAN/%1/statsInterval").arg(deviceKey), page.comboStatsInterval->currentData().toInt());
    }
    
    QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("所有设备的设置已保存"));
    accept();
}

void ProtocolConfigDialog::applyDeviceSettings(DeviceConfigPage &page)
{
    if (!page.connection) {
        QMessageBox::warning(this, QStringLiteral("设备已断开"),
            QStringLiteral("设备 %1 已断开，无法应用设置").arg(page.deviceName));
        return;
    }
    
    if (!page.connection->setUCANMode(ZCAN::Mode::UCAN_BATCH)) {
        QMessageBox::warning(this, QStringLiteral("错误"),
            QStringLiteral("设备 %1 应用模式失败").arg(page.deviceName));
        return;
    }
    
    page.connection->setZeroCopyEnabled(page.chkZeroCopy->isChecked());
    
    const int batchSize = page.comboBatchSize->currentData().toInt();
    const int statsInterval = page.comboStatsInterval->currentData().toInt();
    
    page.connection->setUCANBatchSize(batchSize);
    page.connection->setUCANStatsInterval(statsInterval);
    
    const quint8 maxFrames = static_cast<quint8>(qBound(1, batchSize, 50));
    constexpr quint8 timeoutMs = 5;
    page.connection->configBatch(maxFrames, timeoutMs);
}
