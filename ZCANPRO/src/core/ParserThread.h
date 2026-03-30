#ifndef PARSERTHREAD_H
#define PARSERTHREAD_H

#include <QThread>
#include <QVector>
#include <QByteArray>
#include <atomic>
#include "CANFrame.h"
#include "zcan/zcan_types.h"
#include "SerialConnection.h"
#include "SpscQueue.h"

namespace ZCAN { class ZCANBatchParser; }

class ParserThread final : public QThread
{
    Q_OBJECT

public:
    explicit ParserThread(QObject *parent = nullptr);
    ~ParserThread() override;

    void stop();
    void enqueue(QByteArray &&data);
    void requestReset();
    void requestMode(ZCAN::Mode mode);
    void requestZeroCopyEnabled(bool enabled);

signals:
    void framesParsed(const QVector<CANFrame> &frames, int totalBytes, bool fromUcan);
    void validationOk();
    void deviceInfoParsed(int buildNum, int singleWireMode);
    void firmwareStatisticsParsed(const SerialConnection::FirmwareStatistics &stats);
    void ucanQueryParsed(bool supported, quint32 capabilities, const QString &firmwareVersion);
    void ucanModeParsed(ZCAN::Mode mode);
    void parseError(const QString &message);

protected:
    void run() override;

private:
    enum class StreamState {
        Idle,
        Type,
        Flags,
        Seq,
        LenLo,
        LenHi,
        Payload
    };

    void resetStream();
    void processStreamByte(ZCAN::ZCANBatchParser& batchParser, quint8 c);
    void dispatchPacket(ZCAN::ZCANBatchParser& batchParser);

    SpscQByteArrayQueue<2048> m_queue;
    std::atomic<bool> m_stop{false};
    std::atomic<bool> m_overflow{false};
    std::atomic<bool> m_resetRequested{false};
    std::atomic<int> m_requestedMode{-1};
    std::atomic<bool> m_requestedZeroCopy{true};

    StreamState m_stream{StreamState::Idle};
    quint8 m_type{0};
    quint8 m_flags{0};
    quint8 m_seq{0};
    quint16 m_need{0};
    quint16 m_idx{0};
    QByteArray m_payload{};
};

#endif
