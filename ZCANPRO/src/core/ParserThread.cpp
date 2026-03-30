#include "ParserThread.h"
#include "zcan/ZCANBatchParser.h"
#include <QDateTime>

namespace {
constexpr quint8 UCAN_MAGIC = 0xA6;
constexpr quint8 UCAN_MSG_HELLO_RSP = 0x02;
constexpr quint8 UCAN_MSG_ACK = 0x03;
constexpr quint8 UCAN_MSG_CH_STATE = 0x12;
constexpr quint8 UCAN_MSG_RX_BATCH = 0x21;
constexpr quint8 UCAN_MSG_STATS_RSP = 0x31;
}

ParserThread::ParserThread(QObject *parent)
    : QThread(parent)
{
}

ParserThread::~ParserThread()
{
    stop();
    wait();
}

void ParserThread::stop()
{
    m_stop.store(true, std::memory_order_release);
}

void ParserThread::enqueue(QByteArray &&data)
{
    if (!m_queue.push(std::move(data))) {
        m_overflow.store(true, std::memory_order_release);
    }
}

void ParserThread::requestReset()
{
    m_resetRequested.store(true, std::memory_order_release);
}

void ParserThread::requestMode(ZCAN::Mode mode)
{
    m_requestedMode.store(static_cast<int>(mode), std::memory_order_release);
}

void ParserThread::requestZeroCopyEnabled(bool enabled)
{
    m_requestedZeroCopy.store(enabled, std::memory_order_release);
}

void ParserThread::resetStream()
{
    m_stream = StreamState::Idle;
    m_type = 0;
    m_flags = 0;
    m_seq = 0;
    m_need = 0;
    m_idx = 0;
    m_payload.clear();
}

void ParserThread::dispatchPacket(ZCAN::ZCANBatchParser& batchParser)
{
    switch (m_type) {
    case UCAN_MSG_HELLO_RSP:
        if (m_payload.size() >= 6) {
            const quint8 major = static_cast<quint8>(m_payload[0]);
            const quint8 minor = static_cast<quint8>(m_payload[1]);
            const quint32 caps = static_cast<quint8>(m_payload[2])
                | (static_cast<quint32>(static_cast<quint8>(m_payload[3])) << 8)
                | (static_cast<quint32>(static_cast<quint8>(m_payload[4])) << 16)
                | (static_cast<quint32>(static_cast<quint8>(m_payload[5])) << 24);
            emit ucanQueryParsed(true, caps, QStringLiteral("UCAN %1.%2").arg(major).arg(minor));
            emit validationOk();
        }
        break;
    case UCAN_MSG_ACK:
        emit validationOk();
        break;
    case UCAN_MSG_CH_STATE:
        if (m_payload.size() >= 2) {
            emit deviceInfoParsed(static_cast<quint8>(m_payload[0]), static_cast<quint8>(m_payload[1]));
        }
        break;
    case UCAN_MSG_STATS_RSP:
        if (m_payload.size() >= 24) {
            SerialConnection::FirmwareStatistics stats{};
            auto u32 = [](const QByteArray& p, int o) -> quint32 {
                return static_cast<quint32>(static_cast<quint8>(p[o]))
                    | (static_cast<quint32>(static_cast<quint8>(p[o + 1])) << 8)
                    | (static_cast<quint32>(static_cast<quint8>(p[o + 2])) << 16)
                    | (static_cast<quint32>(static_cast<quint8>(p[o + 3])) << 24);
            };
            stats.can1Rx = u32(m_payload, 0);
            stats.can1Tx = u32(m_payload, 4);
            stats.can1Err = u32(m_payload, 8);
            stats.can2Rx = u32(m_payload, 12);
            stats.can2Tx = u32(m_payload, 16);
            stats.can2Err = u32(m_payload, 20);
            stats.updatedAtMs = QDateTime::currentMSecsSinceEpoch();
            emit firmwareStatisticsParsed(stats);
        }
        break;
    case UCAN_MSG_RX_BATCH:
        batchParser.reset();
        if (!batchParser.parseZeroCopy(m_payload)) {
            emit parseError(QStringLiteral("UCAN RX_BATCH parse failed"));
        }
        break;
    default:
        break;
    }
}

void ParserThread::processStreamByte(ZCAN::ZCANBatchParser& batchParser, quint8 c)
{
    switch (m_stream) {
    case StreamState::Idle:
        if (c == UCAN_MAGIC) m_stream = StreamState::Type;
        break;
    case StreamState::Type:
        m_type = c;
        m_stream = StreamState::Flags;
        break;
    case StreamState::Flags:
        m_flags = c;
        m_stream = StreamState::Seq;
        break;
    case StreamState::Seq:
        m_seq = c;
        m_stream = StreamState::LenLo;
        break;
    case StreamState::LenLo:
        m_need = c;
        m_stream = StreamState::LenHi;
        break;
    case StreamState::LenHi:
        m_need |= static_cast<quint16>(c) << 8;
        m_idx = 0;
        m_payload.clear();
        if (m_need == 0) {
            dispatchPacket(batchParser);
            m_stream = StreamState::Idle;
        } else {
            m_payload.resize(m_need);
            m_stream = StreamState::Payload;
        }
        break;
    case StreamState::Payload:
        m_payload[m_idx++] = static_cast<char>(c);
        if (m_idx >= m_need) {
            dispatchPacket(batchParser);
            m_stream = StreamState::Idle;
        }
        break;
    }
}

void ParserThread::run()
{
    ZCAN::ZCANBatchParser batchParser(nullptr);

    QObject::connect(&batchParser, &ZCAN::ZCANBatchParser::parsingComplete,
                     [this](const QVector<CANFrame> &frames) {
                         int bytes = 0;
                         for (const auto& f : frames) bytes += f.data.size();
                         emit framesParsed(frames, bytes, true);
                     });
    QObject::connect(&batchParser, &ZCAN::ZCANBatchParser::parsingError,
                     [this](const QString& msg) { emit parseError(msg); });

    resetStream();
    while (!m_stop.load(std::memory_order_acquire)) {
        const int requestedMode = m_requestedMode.exchange(-1, std::memory_order_acq_rel);
        if (requestedMode != -1) {
            emit ucanModeParsed(static_cast<ZCAN::Mode>(requestedMode));
        }

        const bool reset = m_resetRequested.exchange(false, std::memory_order_acq_rel);
        const bool overflow = m_overflow.exchange(false, std::memory_order_acq_rel);
        if (reset || overflow) {
            m_queue.clear();
            resetStream();
            if (overflow) emit parseError(QStringLiteral("Parser queue overflow"));
        }

        QByteArray chunk;
        bool hadData = false;
        while (m_queue.pop(chunk)) {
            hadData = true;
            for (int i = 0; i < chunk.size(); ++i) {
                processStreamByte(batchParser, static_cast<quint8>(chunk[i]));
            }
        }

        if (!hadData) msleep(1);
    }
}
