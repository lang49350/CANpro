#include "SerialConnection.h"
#include "ParserThread.h"
#include <QDateTime>
#include "zcan/ZCANStatistics.h"
#include "zcan/zcan_types.h"
#include "LogManager.h"

#ifndef SERIALCONNECTION_RX_STATS_LOGGING
#define SERIALCONNECTION_RX_STATS_LOGGING 0
#endif

namespace {
constexpr quint8 kUcanMagic = 0xA6;
constexpr quint8 kMsgHelloReq = 0x01;
constexpr quint8 kMsgChSet = 0x10;
constexpr quint8 kMsgChGet = 0x11;
constexpr quint8 kMsgStreamSet = 0x14;
constexpr quint8 kMsgTxBatch = 0x20;
constexpr quint8 kMsgStatsGet = 0x30;
constexpr quint8 kBatchFlagHasRecordBytes = 0x01;

quint8 nextSeq()
{
    static quint8 seq = 0;
    return seq++;
}

quint8 bytesToDlc(quint8 bytes)
{
    if (bytes <= 8) return bytes;
    if (bytes <= 12) return 9;
    if (bytes <= 16) return 10;
    if (bytes <= 20) return 11;
    if (bytes <= 24) return 12;
    if (bytes <= 32) return 13;
    if (bytes <= 48) return 14;
    return 15;
}

void appendUcan(QByteArray& out, quint8 type, quint8 flags = 0, const QByteArray& payload = QByteArray())
{
    const quint16 len = static_cast<quint16>(payload.size());
    out.append(static_cast<char>(kUcanMagic));
    out.append(static_cast<char>(type));
    out.append(static_cast<char>(flags));
    out.append(static_cast<char>(nextSeq()));
    out.append(static_cast<char>(len & 0xFF));
    out.append(static_cast<char>((len >> 8) & 0xFF));
    out.append(payload);
}

QByteArray buildTxBatchPayload(const CANFrame &frame)
{
    QByteArray payload;
    payload.reserve(10 + frame.data.size());
    payload.append(char(kBatchFlagHasRecordBytes)); // batch flags
    payload.append(char(1)); // frame count
    payload.append(char(0)); // base ts
    payload.append(char(0));
    payload.append(char(0));
    payload.append(char(0));
    payload.append(char(0)); // dropped
    payload.append(char(0));

    quint32 idFlags = frame.id & 0x1FFFFFFF;
    if (frame.isExtended) idFlags |= 0x80000000U;
    if (frame.isCanFD) idFlags |= 0x40000000U;
    if (frame.isBRS) idFlags |= 0x20000000U;
    if (frame.isESI) idFlags |= 0x10000000U;

    const quint8 dlc = bytesToDlc(static_cast<quint8>(frame.length));
    const quint8 meta = static_cast<quint8>(((frame.channel & 0x03) << 6) | (dlc & 0x0F));

    payload.append(char(idFlags & 0xFF));
    payload.append(char((idFlags >> 8) & 0xFF));
    payload.append(char((idFlags >> 16) & 0xFF));
    payload.append(char((idFlags >> 24) & 0xFF));
    payload.append(char(meta));
    payload.append(char(0)); // dt_us
    payload.append(char(0));
    payload.append(frame.data.left(frame.length));

    const quint16 recordsBytes = static_cast<quint16>(payload.size() - 10);
    payload[8] = static_cast<char>(recordsBytes & 0xFF);
    payload[9] = static_cast<char>((recordsBytes >> 8) & 0xFF);
    return payload;
}
}

// ============================================================================
// SerialConnection
// ============================================================================

SerialConnection::SerialConnection(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_baudRate(2000000)
    , m_isConnected(false)
    , m_batchTimer(nullptr)
    , m_batchSize(4000)
    , m_heartbeatTimer(nullptr)
    , m_validationCounter(10)
    , m_sendFlushTimer(nullptr)
    , m_sendQueueTimer(nullptr)
    , m_totalBytesReceived(0)
    , m_totalBytesSent(0)
    , m_totalFramesReceived(0)
    , m_rxStatsTimer(nullptr)
    , m_intervalBytesReceived(0)
    , m_intervalReadCalls(0)
    , m_intervalFramesParsed(0)
    , m_intervalParseErrors(0)
    , m_ucanStatistics(nullptr)
    , m_ucanSupported(true)
    , m_ucanMode(ZCAN::Mode::UCAN_BATCH)
    , m_zeroCopyEnabled(true)
    , m_ucanBatchSize(50)
    , m_ucanStatsIntervalMs(100)
    , m_parserThread(nullptr)
{
    // 对象统一在 open() 中创建，避免 moveToThread 后线程亲和性问题。
    // setUCANBatchSize(m_ucanBatchSize); // 此时还没有 ZCANStatistics 对象
}

SerialConnection::~SerialConnection()
{
    if (QThread::currentThread() == this->thread()) {
        close();
    } else {
        // IO 线程可能已结束：不要再 BlockingQueuedConnection 到已死线程
        stopParserThread();
    }
}

bool SerialConnection::open(const QString &portName, qint32 baudRate)
{
    // 如果当前不在对象线程，切换到对象线程执行 open。
    if (QThread::currentThread() != this->thread()) {
        LOG_WARNING(LogCategory::DEVICE,
                    QStringLiteral("SerialConnection::open called from wrong thread, current=%1 target=%2")
                        .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16)
                        .arg(reinterpret_cast<quintptr>(this->thread()->currentThreadId()), 0, 16));
        // 尝试跨线程调用自身
        bool result = false;
        QMetaObject::invokeMethod(this, [this, &result, portName, baudRate]() {
            result = open(portName, baudRate);
        }, Qt::BlockingQueuedConnection);
        return result;
    }

    if (m_isConnected) {
        close();
    }
    
    // 确保对象在当前线程创建
    if (!m_serialPort) {
        m_serialPort = new QSerialPort(this);
        m_serialPort->setDataTerminalReady(true);
        m_serialPort->setRequestToSend(true);
    }
    
    if (!m_batchTimer) {
        m_batchTimer = new QTimer(this);
        m_batchTimer->setInterval(300);
        connect(m_batchTimer, &QTimer::timeout, this, &SerialConnection::onBatchTimeout);
    }
    
    if (!m_heartbeatTimer) {
        m_heartbeatTimer = new QTimer(this);
        m_heartbeatTimer->setInterval(2000);
        connect(m_heartbeatTimer, &QTimer::timeout, this, &SerialConnection::onHeartbeatTimeout);
    }
    
    if (!m_sendFlushTimer) {
        m_sendFlushTimer = new QTimer(this);
        m_sendFlushTimer->setInterval(SEND_FLUSH_INTERVAL);
        m_sendFlushTimer->setSingleShot(true);
        connect(m_sendFlushTimer, &QTimer::timeout, this, &SerialConnection::onSendBufferFlush);
        m_sendBuffer.reserve(SEND_BUFFER_SIZE);
    }
    
    if (!m_sendQueueTimer) {
        m_sendQueueTimer = new QTimer(this);
        m_sendQueueTimer->setInterval(1); // 每 1ms 处理一次发送队列
        connect(m_sendQueueTimer, &QTimer::timeout, this, &SerialConnection::processSendQueue);
    }
    
    if (!m_rxStatsTimer) {
        m_rxStatsTimer = new QTimer(this);
        m_rxStatsTimer->setInterval(1000);
        connect(m_rxStatsTimer, &QTimer::timeout, this, &SerialConnection::onRxStatsTimeout);
    }
    
    if (!m_ucanStatistics) {
        m_ucanStatistics = new ZCAN::ZCANStatistics(this);
        setUCANBatchSize(m_ucanBatchSize);
    }
    
    m_batchFrames.reserve(12000);
    startParserThread();
    
    m_portName = portName;
    m_baudRate = baudRate;
    
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
    
    // 性能优化：读取缓冲区提升到 4MB（适应双向高负载场景）
    m_serialPort->setReadBufferSize(4096 * 1024);
    
    // open 前先断开旧连接，避免重复连接信号。
    disconnect(m_serialPort, nullptr, this, nullptr);
    
    // 杩炴帴淇″彿
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialConnection::onReadyRead, Qt::DirectConnection);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialConnection::onSerialError);
    
    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_isConnected = true;
        m_totalBytesReceived = 0;
        m_totalBytesSent = 0;
        m_totalFramesReceived = 0;
        m_lastRxActivityMs = QDateTime::currentMSecsSinceEpoch();
        m_validationCounter = 10;
        
        m_serialPort->setDataTerminalReady(true);   // ESP32-S3 需要 DTR=true
        m_serialPort->setRequestToSend(true);       // RTS 也设置为 true
        
        emit connectionStateChanged(true);

        if (m_parserThread) {
            m_parserThread->requestReset();
        }
        
        sendInitSequence();
        
        QTimer::singleShot(1000, this, [this]() {
            m_batchTimer->start();
            m_heartbeatTimer->start();
            if (m_sendQueueTimer) {
                m_sendQueueTimer->start();
            }
        });

        if (m_rxStatsTimer && !m_rxStatsTimer->isActive()) {
            m_intervalBytesReceived = 0;
            m_intervalReadCalls = 0;
            m_intervalFramesParsed = 0;
            m_intervalParseErrors = 0;
            m_rxStatsTimer->start();
        }
        
        return true;
    } else {
        QString error = m_serialPort->errorString();
        emit errorOccurred("无法打开串口: " + error);
        close(); // 已 startParserThread，需停掉解析线程避免悬空后台线程
        return false;
    }
}

void SerialConnection::close()
{
    // 璺ㄧ嚎绋嬪畨鍏ㄤ繚鎶?
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, [this]() {
            close();
        }, Qt::BlockingQueuedConnection);
        return;
    }

    if (m_serialPort->isOpen()) {
        if (m_batchTimer) {
            m_batchTimer->stop();
        }
        if (m_heartbeatTimer) {
            m_heartbeatTimer->stop();
        }
        if (m_sendQueueTimer) {
            m_sendQueueTimer->stop();
        }
        if (m_rxStatsTimer) {
            m_rxStatsTimer->stop();
        }
        if (m_sendFlushTimer) {
            m_sendFlushTimer->stop();
        }
        
        // 鍒锋柊鍙戦€佺紦鍐插尯
        discardPendingSendData();
        
        if (!m_batchFrames.isEmpty()) {
            emit framesReceived(m_batchFrames);
            m_batchFrames.clear();
        }

        // 先断开可读回调，避免关端口过程中再入队 Parser；再关端口截断新数据
        disconnect(m_serialPort, &QSerialPort::readyRead, this, &SerialConnection::onReadyRead);
        m_serialPort->close();
        m_isConnected = false;
        m_lastRxActivityMs = 0;

        // ParserThread -> SerialConnection 为跨线程 QueuedConnection；若在解析线程仍 emit
        // 时主线程已销毁本对象的 IO 线程，会向已死事件循环投递槽，易导致闪退。
        stopParserThread();

        emit connectionStateChanged(false);
    } else {
        stopParserThread();
    }
}

bool SerialConnection::isOpen() const
{
    return m_isConnected && m_serialPort->isOpen();
}

void SerialConnection::enqueueSendFrame(const CANFrame &frame)
{
    if (!m_isConnected) return;

    QByteArray buffer;
    appendUcan(buffer, kMsgTxBatch, 0, buildTxBatchPayload(frame));
    m_sendQueue.push(std::move(buffer));
}

void SerialConnection::enqueueSendFrames(const QVector<CANFrame> &frames)
{
    if (!m_isConnected || frames.isEmpty()) return;

    for (const CANFrame &frame : frames) {
        QByteArray packet;
        appendUcan(packet, kMsgTxBatch, 0, buildTxBatchPayload(frame));
        m_sendQueue.push(std::move(packet));
    }
}

void SerialConnection::processSendQueue()
{
    if (!isOpen()) return;
    
    QByteArray chunk;
    bool hasData = false;
    
    // 从无锁队列中取出待发送数据
    while (m_sendQueue.pop(chunk)) {
        m_sendBuffer.append(chunk);
        hasData = true;
    }
    
    if (hasData) {
        if (m_sendBuffer.size() >= SEND_BUFFER_SIZE) {
            onSendBufferFlush();
        } else if (!m_sendFlushTimer->isActive()) {
            m_sendFlushTimer->start();
        }
    }
}

bool SerialConnection::sendFrame(const CANFrame &frame)
{
    if (!isOpen()) {
        LOG_WARNING(LogCategory::DEVICE, QStringLiteral("Cannot send CAN frame because serial port is closed: %1").arg(m_portName));
        return false;
    }
    
    QByteArray out;
    appendUcan(out, kMsgTxBatch, 0, buildTxBatchPayload(frame));
    
    m_sendBuffer.append(out);
    
    if (m_sendBuffer.size() >= SEND_BUFFER_SIZE) {
        onSendBufferFlush();
    } else if (!m_sendFlushTimer->isActive()) {
        m_sendFlushTimer->start();
    }
    
    return true;
}

bool SerialConnection::sendFrameImmediate(const CANFrame &frame)
{
    if (!isOpen()) {
        LOG_WARNING(LogCategory::DEVICE, QStringLiteral("Cannot immediately send CAN frame because serial port is closed: %1").arg(m_portName));
        return false;
    }

    QByteArray out;
    appendUcan(out, kMsgTxBatch, 0, buildTxBatchPayload(frame));
    return writeSerialData(out, true);
}

bool SerialConnection::sendCANFDFrame(const CANFrame &frame)
{
    if (!isOpen()) {
        return false;
    }

    QByteArray buffer;
    appendUcan(buffer, kMsgTxBatch, 0, buildTxBatchPayload(frame));
    m_sendBuffer.append(buffer);
    
    // 如果缓冲区已满，立即刷新
    if (m_sendBuffer.size() >= SEND_BUFFER_SIZE) {
        onSendBufferFlush();
    } else if (!m_sendFlushTimer->isActive()) {
        // 启动定时器，批量发送
        m_sendFlushTimer->start();
    }
    
    return true;
}

bool SerialConnection::sendFrames(const QVector<CANFrame> &frames)
{
    if (!isOpen() || frames.isEmpty()) {
        return false;
    }
    
    for (const CANFrame &frame : frames) {
        QByteArray out;
        appendUcan(out, kMsgTxBatch, 0, buildTxBatchPayload(frame));
        m_sendBuffer.append(out);
        
        // 如果缓冲区已满，立即刷新
        if (m_sendBuffer.size() >= SEND_BUFFER_SIZE) {
            onSendBufferFlush();
        }
    }
    
    // 启动定时器，批量发送剩余数据
    if (!m_sendBuffer.isEmpty() && !m_sendFlushTimer->isActive()) {
        m_sendFlushTimer->start();
    }
    
    return true;
}

void SerialConnection::onSendBufferFlush()
{
    if (m_sendBuffer.isEmpty() || !isOpen()) {
        return;
    }
    
    // 批量写入
    qint64 written = m_serialPort->write(m_sendBuffer);
    if (written < 0) {
        LOG_WARNING(LogCategory::DEVICE,
                    QStringLiteral("Serial write failed: %1 - %2").arg(m_portName, m_serialPort->errorString()));
        return;
    }

    if (written > 0) {
        m_totalBytesSent += written;
        m_sendBuffer.remove(0, static_cast<int>(written));
    }

    if (!m_sendBuffer.isEmpty() && m_sendFlushTimer && !m_sendFlushTimer->isActive()) {
        m_sendFlushTimer->start();
    }
}

bool SerialConnection::setBusConfig(int busNum, quint32 baudRate, bool enabled, bool listenOnly)
{
    // 旧接口，保留兼容，内部转调 configureCANBus
    // 默认使用 NORMAL 模式
    // 若 listenOnly=true，则转为 RX_ONLY(1)
    int workMode = listenOnly ? 1 : 0;
    return configureCANBus(busNum, enabled, baudRate, false, 2000000, workMode);
}

bool SerialConnection::configureCANBus(int busId, bool enabled, quint32 bitrate, bool isFD, quint32 fdBitrate, int workMode)
{
    if (QThread::currentThread() != this->thread()) {
        bool result = false;
        QMetaObject::invokeMethod(this, "configureCANBus", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(bool, result),
                                  Q_ARG(int, busId),
                                  Q_ARG(bool, enabled),
                                  Q_ARG(quint32, bitrate),
                                  Q_ARG(bool, isFD),
                                  Q_ARG(quint32, fdBitrate),
                                  Q_ARG(int, workMode));
        return result;
    }

    if (!isOpen()) {
        return false;
    }

    QByteArray pl;
    quint8 modeBits = 0;
    if (enabled) modeBits |= 0x01;
    if (workMode != 2) modeBits |= 0x02;
    if (workMode != 1) modeBits |= 0x04;
    if (workMode == 1) modeBits |= 0x08;
    quint8 protoBits = 0;
    if (isFD) protoBits |= 0x01;
    pl.append((char)busId);
    pl.append((char)modeBits);
    pl.append((char)protoBits);
    pl.append((char)0);
    pl.append((char)(bitrate & 0xFF));
    pl.append((char)((bitrate >> 8) & 0xFF));
    pl.append((char)((bitrate >> 16) & 0xFF));
    pl.append((char)((bitrate >> 24) & 0xFF));
    pl.append((char)(fdBitrate & 0xFF));
    pl.append((char)((fdBitrate >> 8) & 0xFF));
    pl.append((char)((fdBitrate >> 16) & 0xFF));
    pl.append((char)((fdBitrate >> 24) & 0xFF));
    QByteArray buffer;
    appendUcan(buffer, kMsgChSet, 0x01, pl);

    if (writeSerialData(buffer, true)) {
        m_channelModes[busId] = workMode;
        return true;
    }

    return false;
}

bool SerialConnection::configBatch(quint8 maxFrames, quint8 timeoutMs)
{
    if (QThread::currentThread() != this->thread()) {
        bool result = false;
        QMetaObject::invokeMethod(this, "configBatch", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(bool, result),
                                  Q_ARG(quint8, maxFrames),
                                  Q_ARG(quint8, timeoutMs));
        return result;
    }

    if (!isOpen()) {
        return false;
    }

    QByteArray pl;
    pl.append((char)maxFrames);
    pl.append((char)0);
    pl.append((char)(timeoutMs * 100));
    pl.append((char)0);
    pl.append((char)maxFrames);
    pl.append((char)0);
    QByteArray buffer;
    appendUcan(buffer, kMsgStreamSet, 0x01, pl);

    if (writeSerialData(buffer, true)) {
        if (m_ucanStatistics) {
            m_ucanStatistics->setMaxBatchFrames(maxFrames);
        }
        return true;
    }

    return false;
}

bool SerialConnection::requestFirmwareStatistics()
{
    if (QThread::currentThread() != this->thread()) {
        bool result = false;
        QMetaObject::invokeMethod(this, "requestFirmwareStatistics", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(bool, result));
        return result;
    }

    if (!isOpen()) {
        return false;
    }

    QByteArray buffer;
    appendUcan(buffer, kMsgStatsGet, 0x01);
    return writeSerialData(buffer, true);
}

bool SerialConnection::sendCommand(const QString &command)
{
    Q_UNUSED(command);
    // Legacy text command channel is disabled in UCAN-only mode.
    return false;
}

QStringList SerialConnection::availablePorts()
{
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();
    
    for (const QSerialPortInfo &info : infos) {
        QString portInfo = info.portName();
        if (!info.description().isEmpty()) {
            portInfo += " - " + info.description();
        }
        ports.append(portInfo);
    }
    
    return ports;
}

QString SerialConnection::portName() const
{
    return m_portName;
}

qint32 SerialConnection::baudRate() const
{
    return m_baudRate;
}

void SerialConnection::onReadyRead()
{
    QByteArray data = m_serialPort->readAll();
    const int bytes = data.size();
    if (bytes <= 0) {
        return;
    }

    m_totalBytesReceived += bytes;
    m_intervalBytesReceived += bytes;
    m_intervalReadCalls += 1;
    m_lastRxActivityMs = QDateTime::currentMSecsSinceEpoch();
    m_validationCounter = 10;

    if (m_parserThread) {
        m_parserThread->enqueue(std::move(data));
    }
}

void SerialConnection::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }
    
    // 忽略非致命错误（如帧错误、校验错误、超时等）。
    if (error != QSerialPort::ResourceError && error != QSerialPort::PermissionError 
        && error != QSerialPort::DeviceNotFoundError && error != QSerialPort::ReadError 
        && error != QSerialPort::WriteError && error != QSerialPort::UnsupportedOperationError
        && error != QSerialPort::UnknownError) {
        // 这类错误通常无需断开连接
        return;
    }
    
    // 资源类错误（如设备拔出）需要断开重连
    if (error == QSerialPort::ResourceError || error == QSerialPort::PermissionError 
        || error == QSerialPort::DeviceNotFoundError) {
        const QString serialError = m_serialPort ? m_serialPort->errorString() : QStringLiteral("Unknown serial error");
        LOG_WARNING(LogCategory::DEVICE,
                    QStringLiteral("Fatal serial error: code=%1 detail=%2").arg(static_cast<int>(error)).arg(serialError));
        close();
        emit errorOccurred("串口错误: " + serialError);
    }
}

void SerialConnection::onBatchTimeout()
{
    if (!m_batchFrames.isEmpty()) {
        emit framesReceived(m_batchFrames);
        m_totalFramesReceived += m_batchFrames.size();
        m_batchFrames.clear();
    }
}

void SerialConnection::onHeartbeatTimeout()
{
    if (isOpen()) {
        // 数据驱动保活：最近 2 秒内有数据活动则不发送心跳。
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        const bool hasRecentBytes = (m_intervalBytesReceived > 0);
        const bool hasRecentReads = (m_intervalReadCalls > 0);
        const bool hasRecentFrames = (m_intervalFramesParsed > 0);
        const bool hasRecentFirmwareStats = (m_firmwareStatistics.updatedAtMs > 0)
            && (nowMs - m_firmwareStatistics.updatedAtMs < 5000);
        // interval* 每秒在 onRxStatsTimeout 中清零；若本 tick 先清零再执行此处，会误判空闲。用 m_lastRxActivityMs 兜底。
        const bool hasRecentRxWall = (m_lastRxActivityMs > 0) && (nowMs - m_lastRxActivityMs < 3500);

        if (hasRecentBytes || hasRecentReads || hasRecentFrames || hasRecentFirmwareStats || hasRecentRxWall) {
            m_validationCounter = 10;
            return;
        }

        // 链路空闲时发送心跳保活
        sendHeartbeat();
        m_validationCounter--;
        
        // 心跳超时断开机制（20秒无响应）
        if (m_validationCounter <= 0) {
            const qint64 epochMs = QDateTime::currentMSecsSinceEpoch();
            const QString isoLocal = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
            LOG_WARNING(LogCategory::DEVICE,
                        QStringLiteral("[PC-HB-TIMEOUT] ts=%1 epochMs=%2 port=%3 (no device heartbeat ack)")
                            .arg(isoLocal)
                            .arg(epochMs)
                            .arg(m_portName));
            emit errorOccurred("心跳校验超时，设备无响应");
            close();
        }
    }
}

void SerialConnection::sendInitSequence()
{
    QByteArray output;
    appendUcan(output, kMsgHelloReq, 0x01);
    appendUcan(output, kMsgChGet, 0x01, QByteArray(1, char(0)));
    appendUcan(output, kMsgChGet, 0x01, QByteArray(1, char(1)));
    writeSerialData(output, true);
}

void SerialConnection::sendHeartbeat()
{
    QByteArray output;
    appendUcan(output, kMsgHelloReq, 0x01);
    writeSerialData(output, true);

    const qint64 epochMs = QDateTime::currentMSecsSinceEpoch();
    const QString isoLocal = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    qint64 toWrite = m_serialPort ? m_serialPort->bytesToWrite() : -1;
    LOG_INFO(LogCategory::DEVICE,
             QStringLiteral("[PC-HB-TX] ts=%1 epochMs=%2 len=%3 toWrite=%4 port=%5")
                 .arg(isoLocal)
                 .arg(epochMs)
                 .arg(output.size())
                 .arg(toWrite)
                 .arg(m_portName));
}

ZCAN::ZCANStatistics* SerialConnection::getUCANStatistics() const
{
    return m_ucanStatistics;
}

void SerialConnection::onParseError(const QString &error)
{
    m_intervalParseErrors += 1;
    emit errorOccurred(error);
}

void SerialConnection::onRxStatsTimeout()
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        m_intervalBytesReceived = 0;
        m_intervalReadCalls = 0;
        m_intervalFramesParsed = 0;
        m_intervalParseErrors = 0;
        return;
    }

    const qint64 bytes = m_intervalBytesReceived;
    // 如需禁用这些变量，可直接 Q_UNUSED。
    // Q_UNUSED(frames);
    // Q_UNUSED(errors);

    m_intervalBytesReceived = 0;
    m_intervalReadCalls = 0;
    m_intervalFramesParsed = 0;
    m_intervalParseErrors = 0;

    // 简洁的一行统计日志，便于与 ESP32 端数据对齐
    #if SERIALCONNECTION_RX_STATS_LOGGING
    qint64 availB = m_serialPort->bytesAvailable();
    qint64 toWrite = m_serialPort->bytesToWrite();
    qint64 readBuf = m_serialPort->readBufferSize();

    LOG_DEBUG(LogCategory::DEVICE,
              QStringLiteral("[PC-RX] port=%1 availB=%2 read=%3 availA=%4 readBuf=%5 toWrite=%6")
                  .arg(m_portName)
                  .arg(availB)
                  .arg(bytes)
                  .arg(0)
                  .arg(readBuf)
                  .arg(toWrite));
#else
    Q_UNUSED(bytes);
#endif
}

bool SerialConnection::setUCANMode(ZCAN::Mode mode)
{
    Q_UNUSED(mode);
    m_ucanMode = ZCAN::Mode::UCAN_BATCH;
    if (m_parserThread) {
        m_parserThread->requestMode(ZCAN::Mode::UCAN_BATCH);
    }
    return true;
}

void SerialConnection::setZeroCopyEnabled(bool enabled)
{
    m_zeroCopyEnabled = enabled;
    if (m_parserThread) {
        m_parserThread->requestZeroCopyEnabled(enabled);
    }
}

void SerialConnection::setUCANBatchSize(int batchSize)
{
    m_ucanBatchSize = batchSize;

    if (m_ucanStatistics) {
        m_ucanStatistics->setMaxBatchFrames(qBound(1, batchSize, 50));
    }
}

void SerialConnection::setUCANStatsInterval(int intervalMs)
{
    m_ucanStatsIntervalMs = intervalMs;
}

void SerialConnection::startUCANStatistics()
{
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, [this]() {
            startUCANStatistics();
        }, Qt::QueuedConnection);
        return;
    }

    if (m_ucanStatistics && !m_ucanStatistics->isRunning()) {
        m_ucanStatistics->start();
    }
}

void SerialConnection::stopUCANStatistics()
{
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, [this]() {
            stopUCANStatistics();
        }, Qt::QueuedConnection);
        return;
    }

    if (m_ucanStatistics && m_ucanStatistics->isRunning()) {
        m_ucanStatistics->stop();
    }
}

void SerialConnection::startParserThread()
{
    if (m_parserThread) {
        return;
    }

    m_parserThread = new ParserThread(this);

    connect(m_parserThread, &ParserThread::framesParsed, this,
            [this](const QVector<CANFrame> &frames, int totalBytes, bool fromUcan) {
                if (frames.isEmpty()) {
                    return;
                }

                m_batchFrames.append(frames);
                if (m_batchFrames.size() >= m_batchSize) {
                    emit framesReceived(m_batchFrames);
                    m_totalFramesReceived += m_batchFrames.size();
                    m_batchFrames.clear();
                }
                m_intervalFramesParsed += frames.size();
                m_validationCounter = 10;

                if (m_ucanStatistics && m_ucanStatistics->isRunning()) {
                    m_ucanStatistics->recordFramesReceived(frames.size(), totalBytes);
                    if (fromUcan) {
                        m_ucanStatistics->recordBatch(frames.size());
                    }
                }
            });

    connect(m_parserThread, &ParserThread::validationOk, this, [this]() {
        m_validationCounter = 10;
        const qint64 epochMs = QDateTime::currentMSecsSinceEpoch();
        const QString isoLocal = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
        LOG_INFO(LogCategory::DEVICE,
                 QStringLiteral("[PC-HB-RX] ts=%1 epochMs=%2 validationCounter=%3 port=%4 (device UCAN ack)")
                     .arg(isoLocal)
                     .arg(epochMs)
                     .arg(m_validationCounter)
                     .arg(m_portName));
    });

    connect(m_parserThread, &ParserThread::deviceInfoParsed, this,
            [this](int buildNum, int singleWireMode) { emit deviceInfoReceived(buildNum, singleWireMode); });

    connect(m_parserThread, &ParserThread::firmwareStatisticsParsed, this,
            [this](const FirmwareStatistics &stats) {
                m_firmwareStatistics = stats;
                emit firmwareStatisticsUpdated(m_firmwareStatistics);
            });

    connect(m_parserThread, &ParserThread::ucanQueryParsed, this,
            [this](bool supported, quint32, const QString &firmwareVersion) {
                m_ucanSupported = supported;
                m_ucanFirmwareVersion = firmwareVersion;
                m_ucanMode = ZCAN::Mode::UCAN_BATCH;
            });

    connect(m_parserThread, &ParserThread::ucanModeParsed, this,
            [this](ZCAN::Mode mode) { m_ucanMode = mode; });

    connect(m_parserThread, &ParserThread::parseError,
            this, &SerialConnection::onParseError);

    m_parserThread->requestMode(m_ucanMode);
    m_parserThread->requestZeroCopyEnabled(m_zeroCopyEnabled);
    m_parserThread->start();
}

void SerialConnection::stopParserThread()
{
    if (!m_parserThread) {
        return;
    }

    m_parserThread->stop();
    m_parserThread->wait();
    delete m_parserThread;
    m_parserThread = nullptr;
}

bool SerialConnection::writeSerialData(const QByteArray &buffer, bool flushPendingFirst)
{
    if (buffer.isEmpty()) {
        return true;
    }

    if (!isOpen() || !m_serialPort) {
        return false;
    }

    if (flushPendingFirst && !m_sendBuffer.isEmpty()) {
        onSendBufferFlush();
        if (!m_sendBuffer.isEmpty()) {
            m_sendBuffer.append(buffer);
            if (m_sendFlushTimer && !m_sendFlushTimer->isActive()) {
                m_sendFlushTimer->start();
            }
            return false;
        }
    }

    const qint64 written = m_serialPort->write(buffer);
    if (written < 0) {
        LOG_WARNING(LogCategory::DEVICE,
                    QStringLiteral("Serial write failed: %1 - %2").arg(m_portName, m_serialPort->errorString()));
        return false;
    }

    if (written > 0) {
        m_totalBytesSent += written;
    }

    if (written < buffer.size()) {
        const int accepted = static_cast<int>(qMax<qint64>(0, written));
        const QByteArray remainder = buffer.mid(accepted);
        if (!remainder.isEmpty()) {
            m_sendBuffer.prepend(remainder);
            if (m_sendFlushTimer && !m_sendFlushTimer->isActive()) {
                m_sendFlushTimer->start();
            }
        }
        return false;
    }

    return true;
}

void SerialConnection::discardPendingSendData()
{
    m_sendBuffer.clear();

    QByteArray queuedData;
    while (m_sendQueue.pop(queuedData)) {
    }
}
