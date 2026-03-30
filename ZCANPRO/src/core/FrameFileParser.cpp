/**
 * @file FrameFileParser.cpp
 * @brief CAN帧文件解析器实现
 * @author CANAnalyzerPro Team
 * @date 2026-02-25
 */

#include "FrameFileParser.h"
#include "blf/BLFHandler.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QRegularExpression>

/**
 * @brief 检测文件格式
 */
FileFormat FrameFileParser::detectFormat(const QString &filename)
{
    QFileInfo fileInfo(filename);
    QString suffix = fileInfo.suffix().toLower();
    
    if (suffix == "blf") {
        return FileFormat::BLF;
    } else if (suffix == "asc") {
        return FileFormat::ASC;
    } else if (suffix == "csv") {
        return FileFormat::CSV;
    } else if (suffix == "trc") {
        return FileFormat::TRC;
    }
    
    return FileFormat::Unknown;
}

/**
 * @brief 解析文件
 */
bool FrameFileParser::parseFile(const QString &filename, 
                                QVector<CANFrame> &frames, 
                                QString &errorMsg)
{
    FileFormat format = detectFormat(filename);
    
    switch (format) {
        case FileFormat::BLF:
            return parseBLF(filename, frames, errorMsg);
        case FileFormat::ASC:
            return parseASC(filename, frames, errorMsg);
        case FileFormat::CSV:
            return parseCSV(filename, frames, errorMsg);
        default:
            errorMsg = "不支持的文件格式";
            return false;
    }
}

/**
 * @brief 解析BLF文件
 */
bool FrameFileParser::parseBLF(const QString &filename, 
                              QVector<CANFrame> &frames, 
                              QString &errorMsg)
{
    BLF::BLFHandler handler;
    bool success = handler.loadBLF(filename, frames, errorMsg);
    
    return success;
}

/**
 * @brief 解析ASC文件
 */
bool FrameFileParser::parseASC(const QString &filename, 
                              QVector<CANFrame> &frames, 
                              QString &errorMsg)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorMsg = "无法打开文件: " + filename;
        return false;
    }
    
    QTextStream in(&file);
    int lineNumber = 0;
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        lineNumber++;
        
        // 跳过空行和注释
        if (line.isEmpty() || line.startsWith("//") || line.startsWith("date")) {
            continue;
        }
        
        // 解析ASC格式的CAN帧
        // 支持两种格式：
        // 格式1（带通道）: 0.123456 1 100 Rx d 8 01 02 03 04 05 06 07 08
        // 格式2（无通道）: 0.123456 100 Rx d 8 01 02 03 04 05 06 07 08
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        
        if (parts.size() < 5) {
            continue;  // 跳过格式不正确的行
        }
        
        CANFrame frame;
        int partIndex = 0;
        
        // 解析时间戳（秒）
        bool ok;
        double timestamp = parts[partIndex++].toDouble(&ok);
        if (!ok) continue;
        frame.timestamp = static_cast<quint64>(timestamp * 1000000);  // 转换为微秒
        
        // 尝试解析通道号（可选）
        int channel = 0;
        QString nextPart = parts[partIndex];
        
        // 检查下一个字段是否是通道号（数字）还是ID（十六进制）
        bool isChannel = false;
        int tempChannel = nextPart.toInt(&ok);
        if (ok && tempChannel >= 0 && tempChannel <= 10) {
            // 可能是通道号，检查后面是否有ID
            if (partIndex + 1 < parts.size()) {
                QString possibleId = parts[partIndex + 1];
                quint32 testId;
                if (parseHexString(possibleId, testId)) {
                    // 后面确实是ID，所以这是通道号
                    isChannel = true;
                    channel = tempChannel;
                    partIndex++;  // 跳过通道号
                }
            }
        }
        
        frame.channel = static_cast<quint8>(channel);
        
        // 解析ID
        if (partIndex >= parts.size()) continue;
        quint32 id;
        if (!parseHexString(parts[partIndex++], id)) continue;
        frame.id = id;
        
        // 解析方向（Rx/Tx）
        if (partIndex >= parts.size()) continue;
        frame.isReceived = (parts[partIndex++].toLower() == "rx");
        
        // 跳过 'd' 或其他标记
        if (partIndex >= parts.size()) continue;
        partIndex++;  // 跳过 'd'
        
        // 解析数据长度
        if (partIndex >= parts.size()) continue;
        int dataLen = parts[partIndex++].toInt(&ok);
        if (!ok || dataLen < 0 || dataLen > 64) continue;
        frame.length = static_cast<quint8>(dataLen);
        
        // 解析数据字节
        frame.data.clear();
        for (int i = 0; i < dataLen && partIndex < parts.size(); i++, partIndex++) {
            quint32 byte;
            if (parseHexString(parts[partIndex], byte)) {
                frame.data.append(static_cast<char>(byte));
            }
        }
        
        // 设置默认值
        frame.isExtended = (id > 0x7FF);
        
        frames.append(frame);
    }
    
    file.close();
    return true;
}

/**
 * @brief 解析CSV文件
 */
bool FrameFileParser::parseCSV(const QString &filename, 
                              QVector<CANFrame> &frames, 
                              QString &errorMsg)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorMsg = "无法打开文件: " + filename;
        return false;
    }
    
    QTextStream in(&file);
    int lineNumber = 0;
    bool hasHeader = false;
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        lineNumber++;
        
        // 跳过空行
        if (line.isEmpty()) {
            continue;
        }
        
        // 检测并跳过表头
        if (lineNumber == 1 && (line.contains("Time") || line.contains("ID") || line.contains("Data"))) {
            hasHeader = true;
            continue;
        }
        
        // 解析CSV格式的CAN帧
        // 格式示例: 0.123456,0x100,8,01 02 03 04 05 06 07 08
        // 或: 123456,256,8,01 02 03 04 05 06 07 08
        QStringList parts = line.split(',');
        
        if (parts.size() < 3) {
            continue;  // 跳过格式不正确的行
        }
        
        CANFrame frame;
        
        // 解析时间戳
        bool ok;
        double timestamp = parts[0].toDouble(&ok);
        if (!ok) continue;
        
        // 判断时间戳单位（如果小于1000000，假设是秒，否则是微秒）
        if (timestamp < 1000000) {
            frame.timestamp = static_cast<quint64>(timestamp * 1000000);  // 秒转微秒
        } else {
            frame.timestamp = static_cast<quint64>(timestamp);  // 已经是微秒
        }
        
        // 解析ID
        QString idStr = parts[1].trimmed();
        quint32 id;
        if (!parseHexString(idStr, id)) {
            // 尝试作为十进制解析
            id = idStr.toUInt(&ok);
            if (!ok) continue;
        }
        frame.id = id;
        
        // 解析数据长度（如果有）
        int dataLen = 8;  // 默认8字节
        if (parts.size() >= 3) {
            dataLen = parts[2].toInt(&ok);
            if (!ok || dataLen < 0 || dataLen > 64) {
                dataLen = 8;
            }
        }
        frame.length = static_cast<quint8>(dataLen);
        
        // 解析数据字节
        if (parts.size() >= 4) {
            QString dataStr = parts[3].trimmed();
            if (!parseDataBytes(dataStr, frame.data)) {
                // 如果解析失败，填充0
                frame.data = QByteArray(dataLen, 0);
            }
        } else {
            frame.data = QByteArray(dataLen, 0);
        }
        
        // 设置默认值
        frame.channel = 0;
        frame.isReceived = true;
        frame.isExtended = (id > 0x7FF);
        
        frames.append(frame);
    }
    
    file.close();
    return true;
}

/**
 * @brief 解析十六进制字符串
 */
bool FrameFileParser::parseHexString(const QString &str, quint32 &value)
{
    QString cleanStr = str.trimmed();
    
    // 移除0x前缀
    if (cleanStr.startsWith("0x", Qt::CaseInsensitive)) {
        cleanStr = cleanStr.mid(2);
    }
    
    bool ok;
    value = cleanStr.toUInt(&ok, 16);
    return ok;
}

/**
 * @brief 解析数据字节
 */
bool FrameFileParser::parseDataBytes(const QString &str, QByteArray &data)
{
    data.clear();
    
    // 支持多种格式：
    // "01 02 03 04" (空格分隔)
    // "01020304" (连续)
    // "0x01 0x02 0x03 0x04" (带0x前缀)
    
    QString cleanStr = str.trimmed();
    
    // 尝试空格分隔
    QStringList parts = cleanStr.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    
    if (parts.size() > 1) {
        // 空格分隔格式
        for (const QString &part : parts) {
            quint32 byte;
            if (parseHexString(part, byte)) {
                data.append(static_cast<char>(byte & 0xFF));
            } else {
                return false;
            }
        }
    } else {
        // 连续格式
        cleanStr.remove("0x", Qt::CaseInsensitive);
        cleanStr.remove(" ");
        
        if (cleanStr.length() % 2 != 0) {
            return false;  // 必须是偶数个字符
        }
        
        for (int i = 0; i < cleanStr.length(); i += 2) {
            QString byteStr = cleanStr.mid(i, 2);
            bool ok;
            quint8 byte = byteStr.toUInt(&ok, 16);
            if (!ok) {
                return false;
            }
            data.append(static_cast<char>(byte));
        }
    }
    
    return true;
}
