#ifndef ZCAN_BATCH_PARSER_H
#define ZCAN_BATCH_PARSER_H

#include <QObject>
#include <QVector>
#include "../CANFrame.h"

namespace ZCAN {

class ZCANBatchParser : public QObject
{
    Q_OBJECT

public:
    explicit ZCANBatchParser(QObject *parent = nullptr);
    ~ZCANBatchParser() override;

    void reset();
    bool parseZeroCopy(const QByteArray& payload);
    QVector<CANFrame> getFrames();

signals:
    void parsingComplete(const QVector<CANFrame>& frames);
    void parsingError(const QString& message);

private:
    QVector<CANFrame> m_frames;
};

} // namespace ZCAN

#endif // ZCAN_BATCH_PARSER_H
