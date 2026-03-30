#include "FrameStorageEngine.h"
#include <QDir>
#include <QStandardPaths>
#include <QDataStream>
#include <QFileInfo>
#include <algorithm>
#include <exception>
#include <chrono>

FrameStorageEngine* FrameStorageEngine::s_instance = nullptr;

FrameStorageEngine* FrameStorageEngine::instance()
{
    if (!s_instance) {
        s_instance = new FrameStorageEngine();
    }
    return s_instance;
}

FrameStorageEngine::FrameStorageEngine(QObject *parent)
    : QObject(parent)
    , m_running(false)
    , m_nextSequence(1)
    , m_lastCheckpointSequence(0)
    , m_checkpointInterval(1000)
    , m_pendingFrameCounter(0)
    , m_droppedFrameCounter(0)
    , m_maxPendingBatches(256)
    , m_maxPendingFrames(800000)
    , m_ringCapacity(200000)
    , m_ringHead(0)
    , m_ringSize(0)
{
    m_ring.resize(m_ringCapacity);
    initializeStorage();
    m_running = true;
    m_writerThread = std::thread(&FrameStorageEngine::writerLoop, this);
}

FrameStorageEngine::~FrameStorageEngine()
{
    stopWriter();
    writeCheckpoint(true);
    if (m_walFile.isOpen()) {
        m_walFile.flush();
        m_walFile.close();
    }
}

void FrameStorageEngine::initializeStorage()
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::currentPath() + "/storage";
    } else {
        baseDir += "/storage";
    }
    QDir().mkpath(baseDir);
    m_walFilePath = baseDir + "/frames.wal";
    m_checkpointPath = baseDir + "/frames.chk";

    loadCheckpoint();

    m_walFile.setFileName(m_walFilePath);
    if (!m_walFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return;
    }
}

void FrameStorageEngine::loadCheckpoint()
{
    QFile checkpointFile(m_checkpointPath);
    if (!checkpointFile.open(QIODevice::ReadOnly)) {
        m_nextSequence = 1;
        m_lastCheckpointSequence = 0;
        return;
    }

    QDataStream in(&checkpointFile);
    in.setByteOrder(QDataStream::LittleEndian);
    quint64 sequence = 0;
    in >> sequence;
    if (sequence > 0) {
        m_nextSequence = sequence + 1;
        m_lastCheckpointSequence = sequence;
    } else {
        m_nextSequence = 1;
        m_lastCheckpointSequence = 0;
    }
}

void FrameStorageEngine::stopWriter()
{
    bool expected = true;
    if (!m_running.compare_exchange_strong(expected, false)) {
        return;
    }
    m_queueCond.wakeAll();
    if (m_writerThread.joinable()) {
        m_writerThread.join();
    }
}

void FrameStorageEngine::enqueueFrames(const QString &device, const QVector<CANFrame> &frames)
{
    if (frames.isEmpty()) {
        return;
    }

    {
        QMutexLocker locker(&m_queueMutex);
        if (m_pendingBatches.size() >= m_maxPendingBatches ||
            m_pendingFrameCounter + frames.size() > m_maxPendingFrames) {
            m_droppedFrameCounter += static_cast<quint64>(frames.size());
            return;
        }
    }

    BatchItem item;
    item.device = device;
    item.frames = frames;

    QMutexLocker locker(&m_queueMutex);
    m_pendingFrameCounter += item.frames.size();
    m_pendingBatches.append(std::move(item));
    m_queueCond.wakeOne();
}

void FrameStorageEngine::writerLoop()
{
    auto lastWalFlush = std::chrono::steady_clock::now();
    static constexpr auto kWalFlushInterval = std::chrono::milliseconds(100);

    while (m_running.load()) {
        try {
            QVector<BatchItem> localBatches;
            {
                QMutexLocker locker(&m_queueMutex);
                if (m_pendingBatches.isEmpty()) {
                    m_queueCond.wait(&m_queueMutex, 20);
                }
                if (m_pendingBatches.isEmpty()) {
                    continue;
                }
                localBatches.swap(m_pendingBatches);
            }

            int writtenFrames = 0;
            StorageStats globalDelta;
            QHash<QString, StorageStats> deviceDeltas;
            QHash<QString, QHash<int, StorageStats>> deviceChannelDeltas;

            auto accumulateDelta = [](StorageStats &stats, const CANFrame &frame) {
                stats.totalFrames += 1;
                if (frame.isReceived) {
                    stats.rxFrames += 1;
                } else {
                    stats.txFrames += 1;
                }
                if (frame.isError) {
                    stats.errorFrames += 1;
                }
            };

            for (const BatchItem &batch : localBatches) {
                for (const CANFrame &frame : batch.frames) {
                    quint64 sequence = m_nextSequence.fetch_add(1, std::memory_order_relaxed);
                    appendRecord(batch.device, sequence, frame);
                    appendToRing(batch.device, sequence, frame);
                    accumulateDelta(globalDelta, frame);
                    accumulateDelta(deviceDeltas[batch.device], frame);
                    accumulateDelta(deviceChannelDeltas[batch.device][frame.channel], frame);
                    writtenFrames++;
                }
            }

            if (writtenFrames > 0) {
                QMutexLocker statsLocker(&m_statsMutex);
                auto mergeStats = [](StorageStats &dst, const StorageStats &src) {
                    dst.totalFrames += src.totalFrames;
                    dst.rxFrames += src.rxFrames;
                    dst.txFrames += src.txFrames;
                    dst.errorFrames += src.errorFrames;
                };

                mergeStats(m_globalStats, globalDelta);
                for (auto it = deviceDeltas.constBegin(); it != deviceDeltas.constEnd(); ++it) {
                    mergeStats(m_deviceStats[it.key()], it.value());
                }
                for (auto deviceIt = deviceChannelDeltas.constBegin(); deviceIt != deviceChannelDeltas.constEnd(); ++deviceIt) {
                    auto &channelMap = m_deviceChannelStats[deviceIt.key()];
                    const auto &deltaMap = deviceIt.value();
                    for (auto channelIt = deltaMap.constBegin(); channelIt != deltaMap.constEnd(); ++channelIt) {
                        mergeStats(channelMap[channelIt.key()], channelIt.value());
                    }
                }
            }

            const auto now = std::chrono::steady_clock::now();
            if (m_walFile.isOpen() && now - lastWalFlush >= kWalFlushInterval) {
                m_walFile.flush();
                lastWalFlush = now;
            }

            if (writtenFrames > 0) {
                QMutexLocker locker(&m_queueMutex);
                m_pendingFrameCounter -= writtenFrames;
                if (m_pendingFrameCounter < 0) {
                    m_pendingFrameCounter = 0;
                }
            }

            writeCheckpoint(false);
        } catch (const std::exception &) {
            QMutexLocker locker(&m_queueMutex);
            if (!m_pendingBatches.isEmpty()) {
                int dropped = 0;
                for (const BatchItem &batch : m_pendingBatches) {
                    dropped += batch.frames.size();
                }
                m_droppedFrameCounter += static_cast<quint64>(dropped);
                m_pendingBatches.clear();
                m_pendingFrameCounter = 0;
            }
        } catch (...) {
            QMutexLocker locker(&m_queueMutex);
            m_pendingBatches.clear();
            m_pendingFrameCounter = 0;
        }
    }
}

void FrameStorageEngine::appendRecord(const QString &device, quint64 sequence, const CANFrame &frame)
{
    if (!m_walFile.isOpen()) {
        return;
    }

    QByteArray deviceBytes = device.toUtf8();
    QByteArray dataBytes = frame.data;

    quint16 flags = 0;
    if (frame.isExtended) flags |= 0x0001;
    if (frame.isRemote) flags |= 0x0002;
    if (frame.isCanFD) flags |= 0x0004;
    if (frame.isBRS) flags |= 0x0008;
    if (frame.isESI) flags |= 0x0010;
    if (frame.isError) flags |= 0x0020;
    if (frame.isReceived) flags |= 0x0040;

    QDataStream out(&m_walFile);
    out.setByteOrder(QDataStream::LittleEndian);
    out << sequence;
    out << frame.timestamp;
    out << frame.id;
    out << flags;
    out << frame.length;
    out << frame.channel;
    out << static_cast<quint16>(deviceBytes.size());
    out << static_cast<quint16>(dataBytes.size());
    out.writeRawData(deviceBytes.constData(), deviceBytes.size());
    out.writeRawData(dataBytes.constData(), dataBytes.size());
}

void FrameStorageEngine::appendToRing(const QString &device, quint64 sequence, const CANFrame &frame)
{
    QMutexLocker locker(&m_ringMutex);
    int index = (m_ringHead + m_ringSize) % m_ringCapacity;
    if (m_ringSize == m_ringCapacity) {
        index = m_ringHead;
        m_ringHead = (m_ringHead + 1) % m_ringCapacity;
    } else {
        m_ringSize++;
    }
    m_ring[index].sequence = sequence;
    m_ring[index].device = device;
    m_ring[index].frame = frame;
}

void FrameStorageEngine::writeCheckpoint(bool force)
{
    quint64 current = m_nextSequence.load(std::memory_order_relaxed);
    quint64 latest = current > 0 ? (current - 1) : 0;
    if (!force && latest - m_lastCheckpointSequence < static_cast<quint64>(m_checkpointInterval)) {
        return;
    }
    QFile checkpointFile(m_checkpointPath);
    if (!checkpointFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    QDataStream out(&checkpointFile);
    out.setByteOrder(QDataStream::LittleEndian);
    out << latest;
    checkpointFile.flush();
    m_lastCheckpointSequence = latest;
}

QVector<CANFrame> FrameStorageEngine::snapshotFrames(const QString &device, int channel, int maxFrames) const
{
    QVector<CANFrame> result;
    if (maxFrames <= 0) {
        return result;
    }

    QMutexLocker locker(&m_ringMutex);
    result.reserve(maxFrames);

    int matched = 0;
    for (int i = 0; i < m_ringSize && matched < maxFrames; ++i) {
        int idx = (m_ringHead + m_ringSize - 1 - i + m_ringCapacity) % m_ringCapacity;
        const StoredFrame &stored = m_ring[idx];
        if (!device.isEmpty() && stored.device != device) {
            continue;
        }
        if (channel >= 0 && stored.frame.channel != channel) {
            continue;
        }
        result.append(stored.frame);
        matched++;
    }

    std::reverse(result.begin(), result.end());
    return result;
}

QVector<CANFrame> FrameStorageEngine::snapshotFramesSince(const QString &device, int channel, quint64 sinceSequence,
                                                          int maxFrames, quint64 *latestSequenceOut) const
{
    QVector<CANFrame> result;
    if (maxFrames <= 0) {
        if (latestSequenceOut) {
            *latestSequenceOut = sinceSequence;
        }
        return result;
    }

    QMutexLocker locker(&m_ringMutex);

    quint64 latestDeliveredSequence = sinceSequence;
    result.reserve(maxFrames);

    // 从最新向最旧遍历，优先拿最新数据，再反转为时间正序
    for (int i = 0; i < m_ringSize; ++i) {
        int idx = (m_ringHead + m_ringSize - 1 - i + m_ringCapacity) % m_ringCapacity;
        const StoredFrame &stored = m_ring[idx];

        if (stored.sequence <= sinceSequence) {
            break;
        }
        if (!device.isEmpty() && stored.device != device) {
            continue;
        }
        if (channel >= 0 && stored.frame.channel != channel) {
            continue;
        }

        if (result.size() < maxFrames) {
            result.append(stored.frame);
            if (stored.sequence > latestDeliveredSequence) {
                latestDeliveredSequence = stored.sequence;
            }
        }
    }

    std::reverse(result.begin(), result.end());

    if (latestSequenceOut) {
        *latestSequenceOut = latestDeliveredSequence;
    }
    return result;
}

quint64 FrameStorageEngine::latestSequence() const
{
    quint64 current = m_nextSequence.load(std::memory_order_relaxed);
    return current > 0 ? (current - 1) : 0;
}

FrameStorageEngine::StorageStats FrameStorageEngine::queryStats(const QString &device, int channel) const
{
    QMutexLocker locker(&m_statsMutex);

    if (device.isEmpty()) {
        return m_globalStats;
    }

    if (channel < 0) {
        return m_deviceStats.value(device);
    }

    return m_deviceChannelStats.value(device).value(channel);
}

int FrameStorageEngine::pendingBatchCount() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_pendingBatches.size();
}

int FrameStorageEngine::pendingFrameCount() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_pendingFrameCounter;
}

quint64 FrameStorageEngine::droppedFrameCount() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_droppedFrameCounter;
}

QString FrameStorageEngine::walPath() const
{
    return m_walFilePath;
}
