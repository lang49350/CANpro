#ifndef FRAMESTORAGEENGINE_H
#define FRAMESTORAGEENGINE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QFile>
#include <QMutex>
#include <QWaitCondition>
#include <QHash>
#include <QDateTime>
#include <QByteArray>
#include <thread>
#include <atomic>
#include "CANFrame.h"

class FrameStorageEngine : public QObject
{
    Q_OBJECT

public:
    struct StorageStats {
        quint64 totalFrames = 0;
        quint64 rxFrames = 0;
        quint64 txFrames = 0;
        quint64 errorFrames = 0;
    };

    static FrameStorageEngine* instance();
    ~FrameStorageEngine();

    void enqueueFrames(const QString &device, const QVector<CANFrame> &frames);
    QVector<CANFrame> snapshotFrames(const QString &device, int channel, int maxFrames) const;
    QVector<CANFrame> snapshotFramesSince(const QString &device, int channel, quint64 sinceSequence,
                                          int maxFrames, quint64 *latestSequenceOut = nullptr) const;
    quint64 latestSequence() const;
    StorageStats queryStats(const QString &device, int channel) const;
    int pendingBatchCount() const;
    int pendingFrameCount() const;
    quint64 droppedFrameCount() const;
    QString walPath() const;

private:
    explicit FrameStorageEngine(QObject *parent = nullptr);
    FrameStorageEngine(const FrameStorageEngine&) = delete;
    FrameStorageEngine& operator=(const FrameStorageEngine&) = delete;

    struct BatchItem {
        QString device;
        QVector<CANFrame> frames;
    };

    struct StoredFrame {
        quint64 sequence;
        QString device;
        CANFrame frame;
    };

    static FrameStorageEngine *s_instance;

    mutable QMutex m_queueMutex;
    mutable QMutex m_ringMutex;
    QWaitCondition m_queueCond;
    QVector<BatchItem> m_pendingBatches;
    std::atomic<bool> m_running;
    std::thread m_writerThread;

    QFile m_walFile;
    QString m_walFilePath;
    QString m_checkpointPath;
    std::atomic<quint64> m_nextSequence;
    quint64 m_lastCheckpointSequence;
    int m_checkpointInterval;
    int m_pendingFrameCounter;
    quint64 m_droppedFrameCounter;
    int m_maxPendingBatches;
    int m_maxPendingFrames;

    mutable QMutex m_statsMutex;
    StorageStats m_globalStats;
    QHash<QString, StorageStats> m_deviceStats;
    QHash<QString, QHash<int, StorageStats>> m_deviceChannelStats;

    QVector<StoredFrame> m_ring;
    int m_ringCapacity;
    int m_ringHead;
    int m_ringSize;

    void initializeStorage();
    void stopWriter();
    void writerLoop();
    void appendRecord(const QString &device, quint64 sequence, const CANFrame &frame);
    void writeCheckpoint(bool force);
    void loadCheckpoint();
    void appendToRing(const QString &device, quint64 sequence, const CANFrame &frame);
};

#endif // FRAMESTORAGEENGINE_H
