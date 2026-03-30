/**
 * @file CANViewRealTimeSaver.cpp
 * @brief CAN数据实时保存类实现
 * @author CANAnalyzerPro Team
 * @date 2024-02-10
 */

#include "CANViewRealTimeSaver.h"
#include "../core/Logger.h"
#include <QDateTime>
#include <QtEndian>
#include "../core/blf/BLFHandler.h" // 需要引入BLF结构定义

CANViewRealTimeSaver::CANViewRealTimeSaver(QObject *parent)
    : QObject(parent)
    , m_isSaving(false)
    , m_format(FormatASC)
    , m_file(nullptr)
    , m_stream(nullptr)
{
}

CANViewRealTimeSaver::~CANViewRealTimeSaver()
{
    stopSaving();
}

bool CANViewRealTimeSaver::startSaving(const QString &fileName, SaveFormat format)
{
    if (m_isSaving) {
        Logger::warning("已经在保存中");
        return false;
    }
    
    m_format = format;
    m_file = new QFile(fileName);
    
    // BLF是二进制格式，必须以二进制方式打开，不带 Text 标志
    QIODevice::OpenMode mode = QIODevice::WriteOnly;
    if (m_format != FormatBLF) {
        mode |= QIODevice::Text;
    }
    
    if (!m_file->open(mode)) {
        Logger::error("无法创建文件: " + fileName);
        delete m_file;
        m_file = nullptr;
        return false;
    }
    
    if (m_format != FormatBLF) {
        m_stream = new QTextStream(m_file);
    } else {
        m_stream = nullptr; // BLF不使用TextStream
    }
    
    m_isSaving = true;
    
    // 写入文件头
    switch (m_format) {
        case FormatASC:
            writeASCHeader();
            break;
        case FormatBLF:
            writeBLFHeader();
            break;
        case FormatCAN:
            writeCANHeader();
            break;
    }
    
    Logger::info("开始实时保存: " + fileName);
    return true;
}

void CANViewRealTimeSaver::stopSaving()
{
    if (!m_isSaving) {
        return;
    }
    
    // 写入文件尾
    switch (m_format) {
        case FormatASC:
            writeASCFooter();
            break;
        case FormatBLF:
            writeBLFFooter();
            break;
        case FormatCAN:
            writeCANFooter();
            break;
    }
    
    if (m_stream) {
        delete m_stream;
        m_stream = nullptr;
    }
    
    if (m_file) {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
    
    m_isSaving = false;
    Logger::info("停止实时保存");
}

// 批量保存多帧数据
void CANViewRealTimeSaver::saveFrames(const QVector<CANFrame> &frames)
{
    if (!m_isSaving || !m_file || frames.isEmpty()) {
        return;
    }
    
    if (m_format == FormatBLF) {
        // BLF 二进制写入逻辑
        for (const CANFrame &frame : frames) {
            // 直接构造 BLF 对象并写入
            
            // 1. 准备对象数据 (CANObject 或 CANFDObject)
            QByteArray objData;
            uint32_t objType;
            
            if (frame.isCanFD) {
                objType = BLF::CAN_FD_MSG64; // 使用64字节FD消息
                
                BLF::CANFDObject fdObj;
                memset(&fdObj, 0, sizeof(fdObj));
                
                fdObj.channel = qToLittleEndian((uint16_t)frame.channel);
                fdObj.flags = frame.isReceived ? 0 : 1; // bit0: 0=Rx, 1=Tx
                fdObj.dlc = frame.length;
                
                uint32_t id = frame.id;
                if (frame.isExtended) id |= 0x80000000;
                fdObj.id = qToLittleEndian(id);
                
                fdObj.validDataBytes = frame.length;
                fdObj.frameLen = qToLittleEndian((uint32_t)0); // 暂不计算物理长度
                fdObj.bitCount = 0;
                
                uint8_t fdFlags = 0;
                if (frame.isBRS) fdFlags |= 0x01;
                if (frame.isESI) fdFlags |= 0x02;
                fdObj.fdFlags = fdFlags;
                
                // 复制数据
                memcpy(fdObj.data, frame.data.constData(), qMin((int)frame.length, 64));
                
                objData.append((char*)&fdObj, sizeof(fdObj));
            } else {
                objType = BLF::CAN_MSG;
                
                BLF::CANObject canObj;
                memset(&canObj, 0, sizeof(canObj));
                
                canObj.channel = qToLittleEndian((uint16_t)frame.channel);
                canObj.flags = frame.isReceived ? 0 : 1; // bit0: 0=Rx, 1=Tx
                if (frame.isRemote) canObj.flags |= 0x80; // bit7: RTR
                
                canObj.dlc = frame.length;
                
                uint32_t id = frame.id;
                if (frame.isExtended) id |= 0x80000000;
                canObj.id = qToLittleEndian(id);
                
                // 复制数据
                memcpy(canObj.data, frame.data.constData(), qMin((int)frame.length, 8));
                
                objData.append((char*)&canObj, sizeof(canObj));
            }
            
            // 2. 准备对象头 (Base + V1)
            BLF::ObjectHeader header;
            memset(&header, 0, sizeof(header));
            
            // Base Header
            header.base.sig = qToLittleEndian((uint32_t)0x4A424F4C); // "LOBJ"
            header.base.headerSize = qToLittleEndian((uint16_t)32);  // Base(16) + V1(16)
            header.base.headerVersion = qToLittleEndian((uint16_t)1);
            header.base.objSize = qToLittleEndian((uint32_t)(sizeof(BLF::ObjectHeader) + objData.size()));
            header.base.objType = qToLittleEndian(objType);
            
            // V1 Header
            header.v1.flags = qToLittleEndian((uint32_t)1); // TimeOne (1=ns, but we use us * 1000)
            header.v1.clientIdx = 0;
            header.v1.objVer = 0;
            // 这里的 uncompSize 在 V1 头中被用作时间戳（如果是 TimeOne 标志）
            // 注意：BLFHandler 读取时是除以 1000 转微秒，所以这里要乘以 1000 转纳秒
            header.v1.uncompSize = qToLittleEndian((uint64_t)(frame.timestamp * 1000)); 
            
            // 3. 写入文件
            m_file->write((char*)&header, sizeof(header));
            m_file->write(objData);
            
            // 4. 填充对齐 (4字节对齐)
            int padding = objData.size() % 4;
            if (padding > 0) {
                // BLF填充规则：总大小(objSize)不包含填充，但文件流中必须有填充
                // 注意：SavvyCAN实现中，objSize也不包含填充。
                // 写入填充字节
                static const char pad[4] = {0, 0, 0, 0};
                m_file->write(pad, padding);
            }
        }
        m_file->flush();
        return;
    }

    // 文本格式 (ASC/CAN) 写入逻辑
    if (!m_stream) return;
    
    // 预分配字符串缓冲区
    QString buffer;
    buffer.reserve(frames.size() * 100);
    
    for (const CANFrame &frame : frames) {
        switch (m_format) {
            case FormatASC:
                buffer += formatASCFrame(frame);
                break;
            case FormatCAN:
                buffer += formatCANFrame(frame);
                break;
            default: break;
        }
    }
    
    *m_stream << buffer;
    m_stream->flush();
}

// ==================== ASC格式 ====================

void CANViewRealTimeSaver::writeASCHeader()
{
    if (!m_stream) return;
    
    *m_stream << "date " << QDateTime::currentDateTime().toString("ddd MMM dd hh:mm:ss yyyy") << "\n";
    *m_stream << "base hex  timestamps absolute\n";
    *m_stream << "internal events logged\n";
    *m_stream << "Begin Triggerblock\n";
}

void CANViewRealTimeSaver::writeASCFooter()
{
    if (!m_stream) return;
    
    *m_stream << "End Triggerblock\n";
}

// 格式化 ASC 帧为字符串
QString CANViewRealTimeSaver::formatASCFrame(const CANFrame &frame)
{
    QString line;
    line += QString::asprintf("%.6f ", frame.timestamp / 1000000.0);
    line += QString::asprintf("%X ", frame.id);
    line += "Rx ";
    line += QString("d %1 ").arg(frame.length);
    
    for (int i = 0; i < frame.length; i++) {
        line += QString::asprintf("%02X ", static_cast<unsigned char>(frame.data[i]));
    }
    line += "\n";
    
    return line;
}

// ==================== BLF格式 ====================

void CANViewRealTimeSaver::writeBLFHeader()
{
    if (!m_file) return;
    
    BLF::FileHeader header;
    memset(&header, 0, sizeof(header));
    
    header.sig = qToLittleEndian((uint32_t)0x47474F4C); // "LOGG"
    header.headerSize = qToLittleEndian((uint32_t)sizeof(BLF::FileHeader));
    header.appID = 1;
    header.appVerMajor = 1;
    header.appVerMinor = 0;
    header.appVerBuild = 0;
    header.binLogVerMajor = 1;
    header.binLogVerMinor = 0;
    header.binLogVerBuild = 0;
    header.binLogVerPatch = 0;
    header.fileSize = qToLittleEndian((uint64_t)sizeof(BLF::FileHeader)); // 初始大小
    header.uncompressedFileSize = header.fileSize;
    header.countObjs = 0;
    header.countObjsRead = 0;
    
    // 设置时间（这里简化，暂不设置具体时间）
    
    m_file->write((char*)&header, sizeof(header));
}

void CANViewRealTimeSaver::writeBLFFooter()
{
    if (!m_file) return;
    
    // 更新文件头中的文件大小和对象数（如果需要严谨实现，这里应该seek回头部重写）
    // 但对于简单的实时保存，有些工具可能不依赖头部的文件大小
    // 暂不实现 seek 重写，避免文件流操作复杂性
}

// 格式化 BLF 帧为字符串 (不再使用，仅保留接口兼容)
QString CANViewRealTimeSaver::formatBLFFrame(const CANFrame &frame)
{
    Q_UNUSED(frame);
    return QString();
}

// ==================== CAN格式 ====================

void CANViewRealTimeSaver::writeCANHeader()
{
    if (!m_stream) return;
    
    *m_stream << ";$FILEVERSION=1.1\n";
    *m_stream << ";$STARTTIME=" << QDateTime::currentDateTime().toMSecsSinceEpoch() << "\n";
    *m_stream << ";$COLUMNS=N,O,T,I,d,L,D\n";
}

void CANViewRealTimeSaver::writeCANFooter()
{
    if (!m_stream) return;
    
    // CAN格式通常不需要文件尾
}

// 格式化 CAN 帧为字符串
QString CANViewRealTimeSaver::formatCANFrame(const CANFrame &frame)
{
    static int seq = 1;
    QString line;
    
    line += QString::number(seq++) + ",";
    line += "Rx,";
    line += QString::asprintf("%.6f,", frame.timestamp / 1000000.0);
    line += QString::asprintf("%X,", frame.id);
    line += (frame.isExtended ? "X," : "S,");
    line += QString::number(frame.length) + ",";
    
    for (int i = 0; i < frame.length; i++) {
        line += QString::asprintf("%02X", frame.data[i]);
        if (i < frame.length - 1) line += " ";
    }
    line += "\n";
    
    return line;
}
