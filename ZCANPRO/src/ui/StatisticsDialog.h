#ifndef STATISTICSDIALOG_H
#define STATISTICSDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QGroupBox>
#include <QTabWidget>
#include <QMap>
#include <QPointer>
#include "../core/zcan/ZCANStatistics.h"

// 前向声明
class SerialConnection;

/**
 * @brief 性能统计对话框（多标签页版本）
 * 
 * 显示ZCAN协议的详细性能指标：
 * - 吞吐量指标
 * - 延迟指标
 * - 批量传输指标
 * - 压缩指标
 * - 错误指标
 * - 运行状态
 * 
 * 支持多设备：每个设备一个标签页
 */
class StatisticsDialog : public QDialog
{
    Q_OBJECT
    
public:
    // 新构造函数：接受多个设备连接
    explicit StatisticsDialog(const QMap<QString, SerialConnection*> &connections, QWidget *parent = nullptr);
    ~StatisticsDialog();
    
private slots:
    void updateStatistics();
    void onResetClicked();
    void onExportClicked();
    
private:
    // 单个设备的统计页面
    struct DeviceStatsPage {
        QString deviceName;
        ZCAN::ZCANStatistics *statistics;
        QPointer<SerialConnection> connection;
        QWidget *pageWidget;
        qint64 lastFwRequestMs;
        
        // 吞吐量指标标签
        QLabel *lblFramesReceived;
        QLabel *lblFramesSent;
        QLabel *lblThroughputFps;
        QLabel *lblThroughputMbps;
        
        // 延迟指标标签
        QLabel *lblAvgLatency;
        QLabel *lblMinLatency;
        QLabel *lblMaxLatency;
        
        // 批量传输指标标签
        QLabel *lblBatchCount;
        QLabel *lblAvgBatchSize;
        QLabel *lblBatchEfficiency;
        
        // 压缩指标标签
        QLabel *lblCompressCount;
        QLabel *lblOriginalSize;
        QLabel *lblCompressedSize;
        QLabel *lblCompressionRatio;
        
        // 错误指标标签
        QLabel *lblFramesError;
        QLabel *lblFramesDropped;
        QLabel *lblErrorRate;
        
        // 运行状态标签
        QLabel *lblUptime;
        QLabel *lblCurrentMode;

        QLabel *lblFwCan1Rx;
        QLabel *lblFwCan1Tx;
        QLabel *lblFwCan1Err;
        QLabel *lblFwCan2Rx;
        QLabel *lblFwCan2Tx;
        QLabel *lblFwCan2Err;
        QLabel *lblFwTotalRx;
        QLabel *lblFwTotalBatches;
        QLabel *lblFwUpdatedAt;
    };
    
    void setupUi();
    QWidget* createDevicePage(const QString &deviceName, ZCAN::ZCANStatistics *statistics);
    QGroupBox* createThroughputGroup(DeviceStatsPage &page);
    QGroupBox* createLatencyGroup(DeviceStatsPage &page);
    QGroupBox* createBatchGroup(DeviceStatsPage &page);
    QGroupBox* createCompressionGroup(DeviceStatsPage &page);
    QGroupBox* createErrorGroup(DeviceStatsPage &page);
    QGroupBox* createRuntimeGroup(DeviceStatsPage &page);
    QGroupBox* createFirmwareGroup(DeviceStatsPage &page);
    
    void updateDeviceStatistics(DeviceStatsPage &page);
    
    QString formatUptime(qint64 ms);
    QString formatBytes(quint64 bytes);
    
    QTabWidget *m_tabWidget;
    QTimer *m_updateTimer;
    QList<DeviceStatsPage> m_devicePages;
};

#endif // STATISTICSDIALOG_H
