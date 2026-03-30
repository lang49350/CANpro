/**
 * @file BLFHandler.h
 * @brief BLF (Binary Logging Format) 文件处理器
 * @author CANAnalyzerPro Team
 * @date 2026-02-25
 * 
 * 基于 SavvyCAN 的 BLF 处理代码移植
 * 支持 Vector CANalyzer/CANoe 的 BLF 文件格式
 */

#ifndef BLFHANDLER_H
#define BLFHANDLER_H

#include "../CANFrame.h"
#include <QString>
#include <QVector>
#include <QByteArray>
#include <stdint.h>

namespace BLF {

// BLF 对象类型枚举
enum ObjectType {
    CAN_MSG = 1,           // 标准 CAN 消息
    CAN_ERR = 2,           // CAN 错误帧
    CAN_OVERLOAD = 3,      // CAN 过载帧
    CONTAINER = 10,        // 容器对象（可能压缩）
    CAN_MSG2 = 86,         // 扩展 CAN 消息（带额外信息）
    CAN_FD_MSG = 100,      // CAN FD 消息
    CAN_FD_MSG64 = 101,    // CAN FD 64字节消息
    CAN_FD_ERR64 = 104     // CAN FD 错误帧
};

// 容器压缩方法
enum CompressionMethod {
    NO_COMPRESSION = 0,    // 无压缩
    ZLIB_COMPRESSION = 2   // zlib 压缩
};

// BLF 文件头结构 (144 字节)
struct FileHeader {
    uint32_t sig;                  // 文件签名 0x47474F4C ("LOGG")
    uint32_t headerSize;           // 文件头大小
    uint8_t  appID;                // 应用程序ID
    uint8_t  appVerMajor;          // 应用程序主版本号
    uint8_t  appVerMinor;          // 应用程序次版本号
    uint8_t  appVerBuild;          // 应用程序构建号
    uint8_t  binLogVerMajor;       // BLF格式主版本号
    uint8_t  binLogVerMinor;       // BLF格式次版本号
    uint8_t  binLogVerBuild;       // BLF格式构建号
    uint8_t  binLogVerPatch;       // BLF格式补丁号
    uint64_t fileSize;             // 文件大小
    uint64_t uncompressedFileSize; // 未压缩文件大小
    uint32_t countObjs;            // 对象总数
    uint32_t countObjsRead;        // 已读对象数
    uint8_t  startTime[16];        // 开始时间
    uint8_t  stopTime[16];         // 结束时间
    uint8_t  unused[72];           // 保留字段
};

// BLF 对象头基础部分 (16 字节)
struct ObjectHeaderBase {
    uint32_t sig;          // 对象签名 0x4A424F4C ("LOBJ")
    uint16_t headerSize;   // 对象头大小
    uint16_t headerVersion;// 对象头版本
    uint32_t objSize;      // 对象总大小（包括头）
    uint32_t objType;      // 对象类型
};

// BLF 对象头 V1 扩展部分 (16 字节)
struct ObjectHeaderV1 {
    uint32_t flags;        // 标志位
    uint16_t clientIdx;    // 客户端索引
    uint16_t objVer;       // 对象版本
    uint64_t uncompSize;   // 未压缩大小（或时间戳）
};

// BLF 对象头 V2 扩展部分 (24 字节)
struct ObjectHeaderV2 {
    uint32_t flags;        // 标志位
    uint16_t timestampStatus; // 时间戳状态
    uint16_t objVer;       // 对象版本
    uint64_t uncompSize;   // 未压缩大小
    uint64_t origTimestamp;// 原始时间戳
};

// BLF 容器对象头 (16 字节) - 对应 SavvyCAN 的 BLF_OBJ_HEADER_CONTAINER
struct ContainerHeader {
    uint16_t compressionMethod; // 压缩方法 (0=无压缩, 2=zlib)
    uint8_t  reserved[6];       // 保留
    uint32_t uncompressedSize;  // 未压缩大小
    uint8_t  reserved2[4];      // 保留
} __attribute__((packed));

// 完整对象头（32字节：BASE 16 + V1 16）
struct ObjectHeader {
    ObjectHeaderBase base;
    ObjectHeaderV1 v1;
};

// CAN 消息对象 (BLF_CAN_MSG, 类型1)
struct CANObject {
    uint16_t channel;      // 通道号
    uint8_t  flags;        // 标志位
    uint8_t  dlc;          // 数据长度码
    uint32_t id;           // CAN ID
    uint8_t  data[8];      // 数据字节
};

// CAN 消息对象2 (BLF_CAN_MSG2, 类型86) - 带额外信息
struct CANObject2 {
    uint16_t channel;      // 通道号
    uint8_t  flags;        // 标志位 (0=Tx, 5=Err, 6=WU, 7=RTR)
    uint8_t  dlc;          // 数据长度码
    uint32_t id;           // CAN ID
    uint8_t  data[8];      // 数据字节
    uint32_t frameLength;  // 帧长度（纳秒，不含IFS/EOF）
    uint8_t  bitCount;     // 位数（含IFS/EOF）
    uint8_t  reserved1;    // 保留
    uint16_t reserved2;    // 保留
};

// CAN FD 消息对象 (BLF_CAN_FD_MSG, 类型100)
struct CANFDObject {
    uint16_t channel;          // 通道号
    uint8_t  flags;            // 标志位
    uint8_t  dlc;              // 数据长度码
    uint32_t id;               // CAN ID
    uint32_t frameLen;         // 帧长度
    uint8_t  bitCount;         // 位数
    uint8_t  fdFlags;          // FD标志 (bit0=BRS, bit1=ESI)
    uint8_t  validDataBytes;   // 有效数据字节数
    uint8_t  reserved[5];      // 保留
    uint8_t  data[64];         // 数据字节（最多64）
};

/**
 * @brief BLF 文件处理器类
 */
class BLFHandler {
public:
    BLFHandler();
    ~BLFHandler();
    
    /**
     * @brief 加载 BLF 文件
     * @param filename 文件路径
     * @param frames 输出的帧列表
     * @param errorMsg 错误信息
     * @return 成功返回 true
     */
    bool loadBLF(const QString &filename, QVector<CANFrame> &frames, QString &errorMsg);
    
private:
    /**
     * @brief 解析容器对象
     * @param data 容器数据（以 ContainerHeader 开头，后跟压缩/未压缩数据）
     * @param frames 输出的帧列表
     * @return 成功返回 true
     */
    bool parseContainer(const QByteArray &data, QVector<CANFrame> &frames);
    
    /**
     * @brief 解析 CAN 消息对象
     * @param data 对象数据（不含对象头）
     * @param objType 对象类型
     * @param timestamp 时间戳（微秒）
     * @param frame 输出的帧
     * @return 成功返回 true
     */
    bool parseCANMessage(const QByteArray &data, uint32_t objType, uint64_t timestamp, CANFrame &frame);
    
    FileHeader m_fileHeader;
};

} // namespace BLF

#endif // BLFHANDLER_H
