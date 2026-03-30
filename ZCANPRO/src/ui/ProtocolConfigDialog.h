#ifndef PROTOCOLCONFIGDIALOG_H
#define PROTOCOLCONFIGDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QMap>
#include <QPointer>

class SerialConnection;

/**
 * UCAN-FD 设备参数：零拷贝、批量帧数、统计间隔（链路协议固定为 UCAN-FD，不可切换）。
 */
class ProtocolConfigDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ProtocolConfigDialog(const QMap<QString, SerialConnection*> &connections, QWidget *parent = nullptr);
    ~ProtocolConfigDialog();
    
private slots:
    void onApplyClicked();
    void onSaveClicked();
    
private:
    struct DeviceConfigPage {
        QString deviceName;
        QPointer<SerialConnection> connection;
        QWidget *pageWidget;
        
        QLabel *lblProtocol;
        QCheckBox *chkZeroCopy;
        QComboBox *comboBatchSize;
        QComboBox *comboStatsInterval;
        
        QLabel *lblDeviceSupport;
        QLabel *lblFirmwareVersion;
    };
    
    void setupUi();
    QWidget* createDevicePage(const QString &deviceName);
    void loadDeviceSettings(DeviceConfigPage &page);
    void applyDeviceSettings(DeviceConfigPage &page);
    
    QTabWidget *m_tabWidget;
    QList<DeviceConfigPage> m_devicePages;
};

#endif // PROTOCOLCONFIGDIALOG_H
