#include "CANFrame.h"

CANFrame::CANFrame()
    : id(0)
    , timestamp(0)
    , length(0)
    , isExtended(false)
    , isRemote(false)
    , isCanFD(false)
    , isError(false)
    , isReceived(true)
    , channel(0)
{
}

CANFrame::~CANFrame()
{
}

CANFrame::CANFrame(const CANFrame &other)
    : id(other.id)
    , data(other.data)
    , timestamp(other.timestamp)
    , length(other.length)
    , isExtended(other.isExtended)
    , isRemote(other.isRemote)
    , isCanFD(other.isCanFD)
    , isError(other.isError)
    , isReceived(other.isReceived)
    , channel(other.channel)
    , direction(other.direction)
    , type(other.type)
{
}

CANFrame& CANFrame::operator=(const CANFrame &other)
{
    if (this != &other) {
        id = other.id;
        data = other.data;
        timestamp = other.timestamp;
        length = other.length;
        isExtended = other.isExtended;
        isRemote = other.isRemote;
        isCanFD = other.isCanFD;
        isError = other.isError;
        isReceived = other.isReceived;
        channel = other.channel;
        direction = other.direction;
        type = other.type;
    }
    return *this;
}

QString CANFrame::getIdString() const
{
    if (isExtended) {
        return QString("%1").arg(id, 8, 16, QChar('0')).toUpper();
    } else {
        return QString("%1").arg(id, 3, 16, QChar('0')).toUpper();
    }
}

QString CANFrame::getDataString() const
{
    QString result;
    for (int i = 0; i < data.size(); ++i) {
        if (i > 0) result += " ";
        result += QString("%1").arg((quint8)data[i], 2, 16, QChar('0')).toUpper();
    }
    return result;
}

QString CANFrame::getTimestampString() const
{
    double seconds = timestamp / 1000000.0;
    return QString::number(seconds, 'f', 6);
}

QString CANFrame::getTypeString() const
{
    if (isError) return "错误帧";
    if (isRemote) return "远程帧";
    if (isCanFD) return "CAN-FD";
    if (isExtended) return "扩展帧";
    return "标准帧";
}

QString CANFrame::getDirectionString() const
{
    return isReceived ? "接收" : "发送";
}

bool CANFrame::isValid() const
{
    if (isError) return false;
    if (length > 64) return false;
    if (data.size() != length) return false;
    if (!isExtended && id > 0x7FF) return false;
    if (isExtended && id > 0x1FFFFFFF) return false;
    return true;
}
