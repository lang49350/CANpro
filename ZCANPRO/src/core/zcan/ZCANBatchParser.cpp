#include "ZCANBatchParser.h"
#include <QDateTime>

namespace ZCAN {

namespace {
static const quint8 DLC_TO_BYTES_TABLE[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64
};

inline quint8 dlcToBytes(quint8 dlc)
{
    if (dlc >= 16) return 64;
    return DLC_TO_BYTES_TABLE[dlc];
}

inline quint32 readLe32(const quint8* p)
{
    return static_cast<quint32>(p[0])
        | (static_cast<quint32>(p[1]) << 8)
        | (static_cast<quint32>(p[2]) << 16)
        | (static_cast<quint32>(p[3]) << 24);
}

inline quint16 readLe16(const quint8* p)
{
    return static_cast<quint16>(p[0]) | (static_cast<quint16>(p[1]) << 8);
}
}

ZCANBatchParser::ZCANBatchParser(QObject *parent)
    : QObject(parent)
{
}

ZCANBatchParser::~ZCANBatchParser() = default;

void ZCANBatchParser::reset()
{
    m_frames.clear();
}

bool ZCANBatchParser::parseZeroCopy(const QByteArray& payload)
{
    // Strict UCAN-FD v1 parsing:
    // flags(1), count(1), baseTs(4), dropped(2), recordsBytes(2), records...
    if (payload.size() < 10) {
        emit parsingError(QStringLiteral("RX_BATCH payload too small"));
        return false;
    }

    const quint8* p = reinterpret_cast<const quint8*>(payload.constData());
    const quint8* end = p + payload.size();

    const quint8 batchFlags = p[0];
    if ((batchFlags & 0x01U) == 0U) {
        emit parsingError(QStringLiteral("RX_BATCH missing records-length flag"));
        return false;
    }
    const quint8 frameCount = p[1];
    const quint32 baseTs = readLe32(p + 2);
    const quint16 recordsBytes = readLe16(p + 8);
    p += 10;

    const quint8* recordsEnd = p + recordsBytes;
    if (recordsEnd > end) {
        emit parsingError(QStringLiteral("RX_BATCH bad records length"));
        return false;
    }

    m_frames.clear();
    m_frames.reserve(frameCount);

    const quint64 hostBaseUs = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch()) * 1000ULL;

    bool truncated = false;
    for (int i = 0; i < frameCount; ++i) {
        if (recordsEnd - p < 7) {
            truncated = true;
            break;
        }

        const quint32 idFlags = readLe32(p);
        const quint8 meta = p[4];
        const quint16 dtUs = readLe16(p + 5);
        p += 7;

        const quint8 dlc = meta & 0x0F;
        const quint8 dataLen = dlcToBytes(dlc);
        if (recordsEnd - p < dataLen) {
            truncated = true;
            break;
        }

        CANFrame frame;
        frame.id = idFlags & 0x1FFFFFFF;
        frame.isExtended = (idFlags & 0x80000000U) != 0;
        frame.isCanFD = (idFlags & 0x40000000U) != 0;
        frame.isBRS = (idFlags & 0x20000000U) != 0;
        frame.isESI = (idFlags & 0x10000000U) != 0;
        frame.channel = (meta >> 6) & 0x03;
        frame.length = dataLen;
        frame.data = QByteArray(reinterpret_cast<const char*>(p), dataLen);
        frame.isReceived = true;
        frame.timestamp = static_cast<quint64>(hostBaseUs + baseTs + dtUs);

        m_frames.append(frame);
        p += dataLen;
    }

    if (truncated) {
        emit parsingError(QStringLiteral("RX_BATCH truncated"));
        return false;
    }

    if (p != recordsEnd) {
        emit parsingError(QStringLiteral("RX_BATCH record alignment error"));
        return false;
    }

    emit parsingComplete(m_frames);
    return true;
}

QVector<CANFrame> ZCANBatchParser::getFrames()
{
    return m_frames;
}

} // namespace ZCAN
