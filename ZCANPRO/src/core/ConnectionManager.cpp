#include "ConnectionManager.h"
#include "DataRouter.h"
#include "LogManager.h"
#include <QDateTime>

namespace {
constexpr qint64 kNotConnectedWarnIntervalMs = 3000;
}

void ConnectionManager::warnDeviceNotConnectedThrottled(const QString &port, const char *kind,
                                                        const QString &message)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const QString key = port + QLatin1Char('|') + QString::fromUtf8(kind);
    const qint64 last = m_lastNotConnectedWarnMs.value(key, 0);
    if (now - last < kNotConnectedWarnIntervalMs) {
        return;
    }
    m_lastNotConnectedWarnMs[key] = now;
    LOG_WARNING(LogCategory::DEVICE, message);
}

void ConnectionManager::clearNotConnectedLogThrottle(const QString &port)
{
    const QString prefix = port + QLatin1Char('|');
    for (auto it = m_lastNotConnectedWarnMs.begin(); it != m_lastNotConnectedWarnMs.end();) {
        if (it.key().startsWith(prefix)) {
            it = m_lastNotConnectedWarnMs.erase(it);
        } else {
            ++it;
        }
    }
}

ConnectionManager* ConnectionManager::s_instance = nullptr;

ConnectionManager::ConnectionManager(QObject *parent)
    : QObject(parent)
{
    LOG_INFO(LogCategory::DEVICE, "ConnectionManager initialized");
}

ConnectionManager::~ConnectionManager()
{
    disconnectAll();
    LOG_INFO(LogCategory::DEVICE, "ConnectionManager destroyed");
}

ConnectionManager* ConnectionManager::instance()
{
    if (!s_instance) {
        s_instance = new ConnectionManager();
    }
    return s_instance;
}

bool ConnectionManager::connectDevice(const QString &port, int baudRate)
{
    if (m_connections.contains(port)) {
        LOG_INFO(LogCategory::DEVICE, QString("Device already connected, reconnecting: %1").arg(port));
        disconnectDevice(port);
    }

    SerialConnection *connection = new SerialConnection(nullptr);
    QThread *thread = new QThread(this);
    connection->moveToThread(thread);
    connect(thread, &QThread::finished, connection, &QObject::deleteLater);
    m_ioThreads[port] = thread;

    bool success = false;

    thread->start();

    QMetaObject::invokeMethod(connection, [connection, &success, port, baudRate]() {
        success = connection->open(port, baudRate);
    }, Qt::BlockingQueuedConnection);

    if (!success) {
        const QString error = QString("无法打开串口 %1").arg(port);
        LOG_WARNING(LogCategory::DEVICE, error);
        emit errorOccurred(port, error);

        thread->quit();
        if (!thread->wait(1000)) {
            LOG_WARNING(LogCategory::DEVICE, QString("Timeout waiting IO thread stop (open failed): %1").arg(port));
            thread->deleteLater();
        } else {
            delete thread;
        }
        m_ioThreads.remove(port);
        return false;
    }

    m_connections[port] = connection;
    setupConnectionSignals(port, connection);
    clearNotConnectedLogThrottle(port);

    if (DataRouter::instance()->hasSubscribers(port)) {
        connection->startUCANStatistics();
        LOG_INFO(LogCategory::DEVICE, QString("Restarted statistics for reconnected device: %1").arg(port));
    }

    LOG_INFO(LogCategory::DEVICE, QString("Device connected: %1 baud %2").arg(port).arg(baudRate));
    emit deviceConnected(port);
    return true;
}

void ConnectionManager::disconnectDevice(const QString &port)
{
    if (!m_connections.contains(port)) {
        LOG_WARNING(LogCategory::DEVICE, QString("Device not connected: %1").arg(port));
        return;
    }

    SerialConnection *connection = m_connections.take(port);
    QThread *thread = m_ioThreads.take(port);

    m_channelRequirements.remove(port);
    m_activeModes.remove(port);

    if (connection) {
        QMetaObject::invokeMethod(connection, [connection]() {
            connection->close();
        }, Qt::BlockingQueuedConnection);
    }

    if (thread) {
        thread->quit();
        if (!thread->wait(1000)) {
            LOG_WARNING(LogCategory::DEVICE, QString("Timeout waiting IO thread stop (disconnect): %1").arg(port));
            thread->deleteLater();
        } else {
            delete thread;
        }
    }

    LOG_INFO(LogCategory::DEVICE, QString("Device disconnected: %1").arg(port));
    emit deviceDisconnected(port);
}

void ConnectionManager::disconnectAll()
{
    const QStringList ports = m_connections.keys();
    for (const QString &port : ports) {
        disconnectDevice(port);
    }
    LOG_INFO(LogCategory::DEVICE, "All devices disconnected");
}

bool ConnectionManager::isConnected(const QString &port) const
{
    SerialConnection *connection = m_connections.value(port, nullptr);
    if (!connection) {
        return false;
    }

    bool isOpen = false;
    QMetaObject::invokeMethod(connection, [connection, &isOpen]() {
        isOpen = connection->isOpen();
    }, Qt::BlockingQueuedConnection);
    return isOpen;
}

QStringList ConnectionManager::connectedDevices() const
{
    return m_connections.keys();
}

QStringList ConnectionManager::availablePorts()
{
    return SerialConnection::availablePorts();
}

bool ConnectionManager::sendFrame(const QString &port, const CANFrame &frame)
{
    if (!m_connections.contains(port)) {
        warnDeviceNotConnectedThrottled(port, "sendFrame",
            QStringLiteral("Cannot send frame, device not connected: %1").arg(port));
        return false;
    }

    SerialConnection *connection = m_connections[port];
    QMetaObject::invokeMethod(connection, [connection, frame]() {
        connection->sendFrame(frame);
    }, Qt::QueuedConnection);
    return true;
}

bool ConnectionManager::sendFrameSync(const QString &port, const CANFrame &frame)
{
    if (!m_connections.contains(port)) {
        warnDeviceNotConnectedThrottled(port, "sendFrameSync",
            QStringLiteral("Cannot synchronously send frame, device not connected: %1").arg(port));
        return false;
    }

    SerialConnection *connection = m_connections[port];
    bool success = false;
    QMetaObject::invokeMethod(connection, [connection, frame, &success]() {
        success = connection->sendFrame(frame);
    }, Qt::BlockingQueuedConnection);
    return success;
}

bool ConnectionManager::sendFrameSyncImmediate(const QString &port, const CANFrame &frame)
{
    if (!m_connections.contains(port)) {
        warnDeviceNotConnectedThrottled(port, "sendFrameImmediate",
            QStringLiteral("Cannot immediately send frame, device not connected: %1").arg(port));
        return false;
    }

    SerialConnection *connection = m_connections[port];
    bool success = false;
    QMetaObject::invokeMethod(connection, [connection, frame, &success]() {
        success = connection->sendFrameImmediate(frame);
    }, Qt::BlockingQueuedConnection);
    return success;
}

void ConnectionManager::enqueueSendFrame(const QString &port, const CANFrame &frame)
{
    if (!m_connections.contains(port)) {
        warnDeviceNotConnectedThrottled(port, "enqueueSendFrame",
            QStringLiteral("Cannot enqueue frame, device not connected: %1").arg(port));
        return;
    }

    m_connections[port]->enqueueSendFrame(frame);
}

void ConnectionManager::enqueueSendFrames(const QString &port, const QVector<CANFrame> &frames)
{
    if (!m_connections.contains(port)) {
        warnDeviceNotConnectedThrottled(port, "enqueueSendFrames",
            QStringLiteral("Cannot enqueue frames, device not connected: %1").arg(port));
        return;
    }

    m_connections[port]->enqueueSendFrames(frames);
}

SerialConnection* ConnectionManager::getConnection(const QString &port) const
{
    return m_connections.value(port, nullptr);
}

bool ConnectionManager::checkModeConflict(const QString &port, int channel, ChannelMode mode, QString &outReason) const
{
    auto portIt = m_channelRequirements.constFind(port);
    if (portIt == m_channelRequirements.constEnd()) {
        return true;
    }

    const auto &channelMap = portIt.value();
    auto channelIt = channelMap.constFind(channel);
    if (channelIt == channelMap.constEnd()) {
        return true;
    }

    const auto &reqs = channelIt.value();
    if (reqs.isEmpty()) {
        return true;
    }

    for (auto it = reqs.begin(); it != reqs.end(); ++it) {
        const ChannelMode existingMode = it.value();

        if (mode == MODE_RX_ONLY && (existingMode == MODE_TX_ONLY || existingMode == MODE_NORMAL)) {
            outReason = QStringLiteral("该通道正在发送或回放，无法开启监听模式。");
            return false;
        }
        if ((mode == MODE_TX_ONLY || mode == MODE_NORMAL) && existingMode == MODE_RX_ONLY) {
            outReason = QStringLiteral("该通道当前处于监听模式，无法开启发送模式。");
            return false;
        }
    }

    return true;
}

ConnectionManager::ChannelMode ConnectionManager::requestChannelMode(const QString &port, int channel, ChannelMode mode, QObject *requester)
{
    // 保持向后兼容：未显式指定 CAN 类型时，默认申请 CAN FD
    return requestChannelMode(port, channel, mode, requester, true);
}

ConnectionManager::ChannelMode ConnectionManager::requestChannelMode(const QString &port,
                                                                      int channel,
                                                                      ChannelMode mode,
                                                                      QObject *requester,
                                                                      bool isFD)
{
    if (port.isEmpty() || channel < 0 || !requester) {
        return MODE_DISABLED;
    }

    QString reason;
    if (!checkModeConflict(port, channel, mode, reason)) {
        LOG_WARNING(LogCategory::DEVICE,
                    QString("Rejected channel mode request: %1 Ch%2 Mode%3 %4")
                        .arg(port)
                        .arg(channel)
                        .arg(static_cast<int>(mode))
                        .arg(reason));
        return currentChannelMode(port, channel);
    }

    LOG_INFO(LogCategory::DEVICE,
             QString("Requester %1 requested channel mode: %2 Ch%3 Mode%4 (isFD=%5)")
                 .arg(reinterpret_cast<quintptr>(requester), 0, 16)
                 .arg(port)
                 .arg(channel)
                 .arg(static_cast<int>(mode))
                 .arg(isFD ? 1 : 0));

    m_channelRequirements[port][channel][requester] = mode;
    m_fdRequirements[port][channel][requester] = isFD;
    reevaluateChannelMode(port, channel);
    return currentChannelMode(port, channel);
}

void ConnectionManager::releaseChannelMode(const QString &port, int channel, QObject *requester)
{
    if (port.isEmpty() || channel < 0 || !requester) {
        return;
    }

    if (m_channelRequirements.contains(port) && m_channelRequirements[port].contains(channel)) {
        if (m_channelRequirements[port][channel].remove(requester)) {
            LOG_INFO(LogCategory::DEVICE, QString("Requester %1 released channel mode: %2 Ch%3").arg(reinterpret_cast<quintptr>(requester), 0, 16).arg(port).arg(channel));
            reevaluateChannelMode(port, channel);
        }
    }

    if (m_fdRequirements.contains(port) && m_fdRequirements[port].contains(channel)) {
        if (m_fdRequirements[port][channel].remove(requester)) {
            // 只要 isFD 需求变化，也需要重新下发
            reevaluateChannelMode(port, channel);
        }
    }
}

ConnectionManager::ChannelMode ConnectionManager::currentChannelMode(const QString &port, int channel) const
{
    if (m_activeModes.contains(port) && m_activeModes[port].contains(channel)) {
        return m_activeModes[port][channel];
    }
    return MODE_DISABLED;
}

void ConnectionManager::reevaluateChannelMode(const QString &port, int channel)
{
    SerialConnection *conn = getConnection(port);
    if (!conn) {
        return;
    }

    ChannelMode finalMode = MODE_DISABLED;
    const auto &reqs = m_channelRequirements[port][channel];
    bool finalIsFD = false;

    // 汇总 CAN 类型需求：只要存在 FD 需求，就切到 FD（FD 通常可兼收 CAN）
    if (m_fdRequirements.contains(port) && m_fdRequirements[port].contains(channel)) {
        const auto &fdReqs = m_fdRequirements[port][channel];
        for (bool isFdReq : fdReqs.values()) {
            if (isFdReq) {
                finalIsFD = true;
                break;
            }
        }
    }

    if (!reqs.isEmpty()) {
        bool hasRx = false;
        bool hasTx = false;
        bool hasNormal = false;

        for (ChannelMode mode : reqs.values()) {
            if (mode == MODE_NORMAL) {
                hasNormal = true;
            } else if (mode == MODE_RX_ONLY) {
                hasRx = true;
            } else if (mode == MODE_TX_ONLY) {
                hasTx = true;
            }
        }

        if (hasNormal || (hasRx && hasTx)) {
            finalMode = MODE_NORMAL;
        } else if (hasRx) {
            finalMode = MODE_RX_ONLY;
        } else if (hasTx) {
            finalMode = MODE_TX_ONLY;
        }
    }

    const ChannelMode oldMode = m_activeModes[port][channel];
    const bool oldIsFD = m_activeIsFD.value(port).value(channel, true);
    if (oldMode != finalMode || oldIsFD != finalIsFD) {
        m_activeModes[port][channel] = finalMode;
        m_activeIsFD[port][channel] = finalIsFD;

        LOG_INFO(LogCategory::DEVICE,
                 QString("Applied channel mode: %1 Ch%2 Mode%3 isFD=%4")
                     .arg(port)
                     .arg(channel)
                     .arg(static_cast<int>(finalMode))
                     .arg(finalIsFD ? 1 : 0));

        if (finalMode != MODE_DISABLED) {
            QMetaObject::invokeMethod(conn, "configureCANBus", Qt::QueuedConnection,
                Q_ARG(int, channel),
                Q_ARG(bool, true),            // enabled
                Q_ARG(quint32, 500000),
                Q_ARG(bool, finalIsFD),
                Q_ARG(quint32, 2000000),
                Q_ARG(int, static_cast<int>(finalMode)));
        } else {
            QMetaObject::invokeMethod(conn, "configureCANBus", Qt::QueuedConnection,
                Q_ARG(int, channel),
                Q_ARG(bool, false),
                Q_ARG(quint32, 500000),
                Q_ARG(bool, finalIsFD),
                Q_ARG(quint32, 2000000),
                Q_ARG(int, 1));
        }

        emit channelModeChanged(port, channel, finalMode);
    }
}

void ConnectionManager::setupConnectionSignals(const QString &port, SerialConnection *connection)
{
    connect(connection, &SerialConnection::connectionStateChanged,
            this, [this, port](bool connected) {
        if (!connected && m_connections.contains(port)) {
            LOG_WARNING(LogCategory::DEVICE, QString("Unexpected disconnect: %1").arg(port));

            SerialConnection *const conn = m_connections.take(port);
            m_channelRequirements.remove(port);
            m_fdRequirements.remove(port);
            m_activeModes.remove(port);
            m_activeIsFD.remove(port);

            if (conn) {
                disconnect(conn, nullptr, this, nullptr);
            }

            QThread *thread = m_ioThreads.take(port);
            if (thread) {
                thread->quit();
                if (!thread->wait(1000)) {
                    LOG_WARNING(LogCategory::DEVICE, QString("Timeout waiting IO thread stop (unexpected disconnect): %1").arg(port));
                    thread->deleteLater();
                } else {
                    delete thread;
                }
            }

            emit deviceDisconnected(port);
        }
    });

    connect(connection, &SerialConnection::framesReceived,
            this, [this, port](const QVector<CANFrame> &frames) {
        emit framesReceived(port, frames);
    });

    connect(connection, &SerialConnection::errorOccurred,
            this, [this, port](const QString &error) {
        LOG_WARNING(LogCategory::DEVICE, QString("Device %1 error: %2").arg(port, error));
        emit errorOccurred(port, error);
    });
}
