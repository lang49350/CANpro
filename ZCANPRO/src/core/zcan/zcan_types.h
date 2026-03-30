#ifndef ZCAN_TYPES_H
#define ZCAN_TYPES_H

#include <QtGlobal>
#include <QVector>
#include "../CANFrame.h"

namespace ZCAN {

// Host runtime now uses UCAN-FD as the only transport protocol.
enum class Mode {
    UCAN_RESERVED = 0,
    UCAN_BATCH = 1,
    UCAN_COMPRESS = 2,
    UCAN_AUTO = 3
};

enum class Command : quint8 {
    HELLO_REQ = 0x01,
    HELLO_RSP = 0x02,
    ACK = 0x03,
    CH_SET = 0x10,
    CH_GET = 0x11,
    CH_STATE = 0x12,
    STREAM_SET = 0x14,
    TX_BATCH = 0x20,
    RX_BATCH = 0x21,
    STATS_GET = 0x30,
    STATS_RSP = 0x31
};

enum class CompressAlgorithm : quint8 {
    NONE = 0,
    DIFFERENTIAL = 1,
    LZ4 = 2,
    RLE = 3
};

#pragma pack(push, 1)
struct BatchHeader {
    quint8 frameCount;
    quint32 baseTimestamp;
    quint8 flags;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BatchFrameData {
    quint32 canId;
    quint8 busLen;
    quint16 timeOffset;
    quint8 data[64];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct CompressHeader {
    quint8 algorithm;
    quint16 originalSize;
    quint16 compressedSize;
};
#pragma pack(pop)

struct DeviceCapabilities {
    bool supportsBatch;
    bool supportsCompress;
    bool supportsNegotiate;
    quint8 maxBatchSize;
    quint32 maxBaudRate;
    quint16 protocolVersion;
};

struct Statistics {
    quint32 framesReceived;
    quint32 framesSent;
    quint32 framesError;
    quint32 framesDropped;
    quint32 busLoad;
};

} // namespace ZCAN

#endif // ZCAN_TYPES_H
