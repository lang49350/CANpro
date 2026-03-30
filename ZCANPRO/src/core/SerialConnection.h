#ifndef SERIALCONNECTION_H
#define SERIALCONNECTION_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QVector>
#include <QDateTime>
#include <QThread>
#include <array>
#include <atomic>
#include "CANFrame.h"
#include "zcan/zcan_types.h"
#include "SpscQueue.h"

namespace ZCAN {
    class ZCANStatistics;
}

class ParserThread;

class SerialConnection : public QObject
{
    Q_OBJECT
    
public:
    explicit SerialConnection(QObject *parent = nullptr);
    ~SerialConnection();
    
    struct FirmwareStatistics {
        quint32 can1Rx = 0;
        quint32 can1Tx = 0;
        quint32 can1Err = 0;
        quint32 can2Rx = 0;
        quint32 can2Tx = 0;
        quint32 can2Err = 0;
        quint32 totalRx = 0;
        quint32 totalBatches = 0;
        qint64 updatedAtMs = 0;
    };
    
    bool open(const QString &portName, qint32 baudRate = 2000000);
    void close();
    bool isOpen() const;
    
    bool sendFrame(const CANFrame &frame);
    bool sendFrameImmediate(const CANFrame &frame);
    bool sendCANFDFrame(const CANFrame &frame);
    bool sendFrames(const QVector<CANFrame> &frames);
    
    void enqueueSendFrame(const CANFrame &frame);
    void enqueueSendFrames(const QVector<CANFrame> &frames);
    bool sendCommand(const QString &command);
    bool setBusConfig(int busNum, quint32 baudRate, bool enabled, bool listenOnly = false);
    Q_INVOKABLE bool configureCANBus(int busId, bool enabled, quint32 bitrate, bool isFD, quint32 fdBitrate, int workMode = 0);
    int getWorkMode(int channel) const { return m_channelModes.value(channel, 0); }
    
    Q_INVOKABLE bool configBatch(quint8 maxFrames, quint8 timeoutMs);
    Q_INVOKABLE bool requestFirmwareStatistics();
    FirmwareStatistics firmwareStatistics() const { return m_firmwareStatistics; }
    
    static QStringList availablePorts();
    
    QString portName() const;
    qint32 baudRate() const;
    
    qint64 totalBytesReceived() const { return m_totalBytesReceived; }
    qint64 totalBytesSent() const { return m_totalBytesSent; }
    quint64 totalFramesReceived() const { return m_totalFramesReceived; }
    
    ZCAN::ZCANStatistics* getUCANStatistics() const;
    bool isUCANSupported() const { return m_ucanSupported; }
    ZCAN::Mode ucanMode() const { return m_ucanMode; }
    bool setUCANMode(ZCAN::Mode mode);
    QString ucanFirmwareVersion() const { return m_ucanFirmwareVersion; }
    bool isZeroCopyEnabled() const { return m_zeroCopyEnabled; }
    void setZeroCopyEnabled(bool enabled);
    void setUCANBatchSize(int batchSize);
    void setUCANStatsInterval(int intervalMs);
    void startUCANStatistics();
    void stopUCANStatistics();
    
    void processSendQueue();
    
signals:
    void framesReceived(const QVector<CANFrame> &frames);
    void connectionStateChanged(bool connected);
    void errorOccurred(const QString &error);
    void deviceInfoReceived(int buildNum, int singleWireMode);
    void batchFramesReceived(const QVector<CANFrame> &frames);
    void firmwareStatisticsUpdated(const FirmwareStatistics &stats);
    
private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);
    void onBatchTimeout();
    void onHeartbeatTimeout();
    void onSendBufferFlush();
    void onParseError(const QString &error);
    void onRxStatsTimeout();
    
private:
    void sendInitSequence();
    void sendHeartbeat();
    void startParserThread();
    void stopParserThread();
    bool writeSerialData(const QByteArray &buffer, bool flushPendingFirst = false);
    void discardPendingSendData();
    
    QSerialPort *m_serialPort;
    QString m_portName;
    qint32 m_baudRate;
    bool m_isConnected;
    QMap<int, int> m_channelModes;
    
    FirmwareStatistics m_firmwareStatistics;
    
    QVector<CANFrame> m_batchFrames;
    QTimer *m_batchTimer;
    int m_batchSize;
    
    QTimer *m_heartbeatTimer;
    int m_validationCounter;
    
    QByteArray m_sendBuffer;
    QTimer *m_sendFlushTimer;
    QTimer *m_sendQueueTimer;
    static const int SEND_FLUSH_INTERVAL = 15;
    static const int SEND_BUFFER_SIZE = 16384;
    
    qint64 m_totalBytesReceived;
    qint64 m_totalBytesSent;
    quint64 m_totalFramesReceived;

    QTimer *m_rxStatsTimer;
    qint64 m_intervalBytesReceived;
    qint64 m_intervalReadCalls;
    quint64 m_intervalFramesParsed;
    qint64 m_intervalParseErrors;
    /** 最近一次从串口读到字节的时间（ms epoch）。与 interval* 不同，不由 onRxStatsTimeout 清零，避免与心跳定时器竞态误判“无活动”。 */
    qint64 m_lastRxActivityMs{0};
    
    ZCAN::ZCANStatistics* m_ucanStatistics;
    bool m_ucanSupported;
    ZCAN::Mode m_ucanMode;
    QString m_ucanFirmwareVersion;
    bool m_zeroCopyEnabled;
    int m_ucanBatchSize;
    int m_ucanStatsIntervalMs;
    
    ParserThread* m_parserThread;
    
    SpscQByteArrayQueue<2048> m_sendQueue;
};

#endif // SERIALCONNECTION_H
