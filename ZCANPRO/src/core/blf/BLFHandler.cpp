/**
 * @file BLFHandler.cpp
 * @brief BLF 文件处理器实现
 * @author CANAnalyzerPro Team
 * @date 2026-02-25
 */

#include "BLFHandler.h"
#include <QFile>
#include <QtEndian>

namespace BLF {

BLFHandler::BLFHandler()
{
    memset(&m_fileHeader, 0, sizeof(m_fileHeader));
}

BLFHandler::~BLFHandler()
{
}

bool BLFHandler::loadBLF(const QString &filename, QVector<CANFrame> &frames, QString &errorMsg)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMsg = "无法打开文件: " + filename;
        return false;
    }
    
    // 读取文件头
    if (file.read((char*)&m_fileHeader, sizeof(FileHeader)) != sizeof(FileHeader)) {
        errorMsg = "文件头读取失败";
        file.close();
        return false;
    }
    
    // 验证文件签名 "LOGG" = 0x47474F4C
    if (qFromLittleEndian(m_fileHeader.sig) != 0x47474F4C) {
        errorMsg = "无效的BLF文件签名";
        file.close();
        return false;
    }
    
    // 循环读取对象
    int objectCount = 0;
    while (!file.atEnd()) {
        // 读取对象头基础部分
        ObjectHeaderBase objHeaderBase;
        if (file.read((char*)&objHeaderBase, sizeof(ObjectHeaderBase)) != sizeof(ObjectHeaderBase)) {
            break; // 文件结束
        }
        
        // 验证对象签名 "LOBJ" = 0x4A424F4C
        if (qFromLittleEndian(objHeaderBase.sig) != 0x4A424F4C) {
            break;
        }
        
        // 读取对象数据（不含已读的基础头）
        uint32_t objSize = qFromLittleEndian(objHeaderBase.objSize);
        int dataSize = objSize - sizeof(ObjectHeaderBase);
        QByteArray objData = file.read(dataSize);
        
        if (objData.size() != dataSize) {
            errorMsg = "对象数据读取不完整";
            file.close();
            return false;
        }
        
        // BLF特殊对齐规则: objSize + (objSize % 4)
        int padding = objSize % 4;
        if (padding > 0) {
            file.read(padding);
        }
        
        uint32_t objType = qFromLittleEndian(objHeaderBase.objType);
        objectCount++;
        
        // 根据对象类型处理
        if (objType == CONTAINER) {
            // 该 BLF 的容器对象数据以 ContainerHeader 开头，后面紧跟压缩/未压缩数据
            parseContainer(objData, frames);
        }
        else if (objType == CAN_MSG || objType == CAN_MSG2 || 
                 objType == CAN_FD_MSG || objType == CAN_FD_MSG64) {
            // 提取时间戳（从 V1 头的 uncompSize 字段）
            ObjectHeaderV1 v1Header;
            if (objData.size() >= (int)sizeof(ObjectHeaderV1)) {
                memcpy(&v1Header, objData.constData(), sizeof(ObjectHeaderV1));
                // BLF 的该字段在这些样本中更像“纳秒”，回放内部用“微秒”
                uint64_t timestamp = qFromLittleEndian(v1Header.uncompSize) / 1000;
                
                // 跳过 V1 头，获取实际数据
                QByteArray payloadData = objData.mid(sizeof(ObjectHeaderV1));
                
                CANFrame frame;
                if (parseCANMessage(payloadData, objType, timestamp, frame)) {
                    frames.append(frame);
                }
            }
        }
    }
    
    file.close();
    return true;
}

bool BLFHandler::parseContainer(const QByteArray &data, QVector<CANFrame> &frames)
{
    // data = 容器特定头(16字节) + 压缩数据
    if (data.size() < (int)sizeof(ContainerHeader)) {
        return false;
    }
    
    // 读取容器头(16字节)
    ContainerHeader containerHeader;
    memcpy(&containerHeader, data.constData(), sizeof(ContainerHeader));
    
    uint16_t compressionMethod = qFromLittleEndian(containerHeader.compressionMethod);
    uint32_t uncompressedSize = qFromLittleEndian(containerHeader.uncompressedSize);
    
    // 容器头之后就是压缩数据(或未压缩数据)
    QByteArray containerPayload = data.mid(sizeof(ContainerHeader));
    QByteArray uncompressedData;
    
    if (compressionMethod == NO_COMPRESSION) {
        uncompressedData = containerPayload;
    }
    else if (compressionMethod == ZLIB_COMPRESSION) {
        QByteArray dataForUncompress;
        dataForUncompress.append((uncompressedSize >> 24) & 0xFF);
        dataForUncompress.append((uncompressedSize >> 16) & 0xFF);
        dataForUncompress.append((uncompressedSize >> 8) & 0xFF);
        dataForUncompress.append(uncompressedSize & 0xFF);
        dataForUncompress.append(containerPayload);
        
        uncompressedData = qUncompress(dataForUncompress);
        
        if (uncompressedData.isEmpty()) {
            return false;
        }
    }
    else {
        return false;
    }
    
    // 解析容器内的对象
    // 关键：容器内对象头是 BASE(16)+V1(16)=32 字节，由 headerVersion 决定
    int pos = 0;
    int containerFrameCount = 0;
    
    // 先跳到第一个有效对象头 (LOBJ = 0x4A424F4C 小端)
    // 有些容器解压后前面可能有几个字节的额外数据,需要搜索
    while (pos + (int)sizeof(ObjectHeaderBase) < uncompressedData.size()) {
        uint32_t sig = qFromLittleEndian(*((const uint32_t*)(uncompressedData.constData() + pos)));
        if (sig == 0x4A424F4C) {
            break;
        }
        pos++;  // 逐字节搜索,不是4字节对齐
    }
    
    if (pos + (int)sizeof(ObjectHeaderBase) >= uncompressedData.size()) {
        return true;
    }
    
    // 循环解析对象 - 参考 SavvyCAN 的逻辑
    int objIndex = 0;
    while (pos + (int)sizeof(ObjectHeaderBase) + (int)sizeof(ObjectHeaderV1) < uncompressedData.size()) {
        // 读取 BASE(16字节) + V1(16字节) = 32字节对象头
        ObjectHeaderBase base;
        ObjectHeaderV1 v1;
        memcpy(&base, uncompressedData.constData() + pos, sizeof(ObjectHeaderBase));
        memcpy(&v1, uncompressedData.constData() + pos + sizeof(ObjectHeaderBase), sizeof(ObjectHeaderV1));
        
        uint32_t sig = qFromLittleEndian(base.sig);
        if (sig != 0x4A424F4C) {
            break;
        }
        
        uint32_t objSize = qFromLittleEndian(base.objSize);
        uint32_t objType = qFromLittleEndian(base.objType);
        uint64_t timestamp = qFromLittleEndian(v1.uncompSize) / 1000;
        
        objIndex++;
        
        // 计算payload位置和大小
        int headerTotalSize = sizeof(ObjectHeaderBase) + sizeof(ObjectHeaderV1);  // 32字节
        int payloadSize = (int)objSize - headerTotalSize;
        
        if (payloadSize < 0 || pos + headerTotalSize + payloadSize > uncompressedData.size()) {
            break;
        }
        
        QByteArray payloadData = uncompressedData.mid(pos + headerTotalSize, payloadSize);
        
        // 解析CAN消息
        if (objType == CAN_MSG || objType == CAN_MSG2 ||
            objType == CAN_FD_MSG || objType == CAN_FD_MSG64) {
            CANFrame frame;
            if (parseCANMessage(payloadData, objType, timestamp, frame)) {
                frames.append(frame);
                containerFrameCount++;
            }
        }
        
        // 跳到下一个对象
        int padding = (int)(objSize % 4);
        pos += (int)objSize + padding;
    }
    
    return true;
}

bool BLFHandler::parseCANMessage(const QByteArray &data, uint32_t objType, 
                                 uint64_t timestamp, CANFrame &frame)
{
    if (objType == CAN_MSG) {
        // BLF_CAN_MSG 使用 CANObject 结构
        if (data.size() < (int)sizeof(CANObject)) {
            return false;
        }
        
        CANObject canObj;
        memcpy(&canObj, data.constData(), sizeof(CANObject));
        
        uint32_t id = qFromLittleEndian(canObj.id);
        frame.id = id & 0x1FFFFFFF;
        frame.isExtended = (id & 0x80000000) != 0;
        frame.channel = qFromLittleEndian(canObj.channel);
        frame.length = canObj.dlc;
        frame.data = QByteArray((char*)canObj.data, canObj.dlc);
        frame.isReceived = (canObj.flags & 0x01) == 0; // Tx=1, Rx=0
        frame.isRemote = (canObj.flags & 0x80) != 0;
        frame.isCanFD = false;
        frame.timestamp = timestamp; // 微秒
        
        return true;
    }
    else if (objType == CAN_MSG2) {
        // BLF_CAN_MSG2 必须使用 CANObject2 结构（带 frameLength、bitCount）
        if (data.size() < (int)sizeof(CANObject2)) {
            return false;
        }
        
        CANObject2 canObj2;
        memcpy(&canObj2, data.constData(), sizeof(CANObject2));
        
        uint32_t id = qFromLittleEndian(canObj2.id);
        frame.id = id & 0x1FFFFFFF;
        frame.isExtended = (id & 0x80000000) != 0;
        frame.channel = qFromLittleEndian(canObj2.channel);
        frame.length = canObj2.dlc;
        frame.data = QByteArray((char*)canObj2.data, canObj2.dlc);
        frame.isReceived = (canObj2.flags & 0x01) == 0;
        frame.isRemote = (canObj2.flags & 0x80) != 0;
        frame.isCanFD = false;
        frame.timestamp = timestamp;
        
        return true;
    }
    else if (objType == CAN_FD_MSG || objType == CAN_FD_MSG64) {
        // CAN FD 消息
        if (data.size() < (int)sizeof(CANFDObject)) {
            return false;
        }
        
        CANFDObject fdObj;
        memcpy(&fdObj, data.constData(), sizeof(CANFDObject));
        
        uint32_t id = qFromLittleEndian(fdObj.id);
        frame.id = id & 0x1FFFFFFF;
        frame.isExtended = (id & 0x80000000) != 0;
        frame.channel = qFromLittleEndian(fdObj.channel);
        frame.length = fdObj.validDataBytes;
        frame.data = QByteArray((char*)fdObj.data, fdObj.validDataBytes);
        frame.isCanFD = true;
        frame.isBRS = (fdObj.fdFlags & 0x01) != 0;
        frame.isESI = (fdObj.fdFlags & 0x02) != 0;
        frame.isReceived = (fdObj.flags & 0x01) == 0;
        frame.timestamp = timestamp;
        
        return true;
    }
    
    return false;
}

} // namespace BLF
