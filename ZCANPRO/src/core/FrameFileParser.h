/**
 * @file FrameFileParser.h
 * @brief CAN帧文件解析器
 * @author CANAnalyzerPro Team
 * @date 2026-02-25
 */

#ifndef FRAMEFILEPARSER_H
#define FRAMEFILEPARSER_H

#include <QString>
#include <QVector>
#include "CANFrame.h"

/**
 * @brief 文件格式枚举
 */
enum class FileFormat {
    Unknown,
    BLF,        // Vector BLF格式
    ASC,        // Vector ASC格式
    CSV,        // CSV格式
    TRC         // PEAK TRC格式
};

/**
 * @brief CAN帧文件解析器
 * 
 * 职责：
 * - 检测文件格式
 * - 解析不同格式的CAN日志文件
 * - 提取CANFrame数据和时间戳
 */
class FrameFileParser
{
public:
    /**
     * @brief 检测文件格式
     * @param filename 文件路径
     * @return 文件格式
     */
    static FileFormat detectFormat(const QString &filename);
    
    /**
     * @brief 解析文件
     * @param filename 文件路径
     * @param frames 输出帧列表
     * @param errorMsg 错误消息
     * @return 是否成功
     */
    static bool parseFile(const QString &filename, 
                         QVector<CANFrame> &frames, 
                         QString &errorMsg);
    
    /**
     * @brief 解析BLF文件
     */
    static bool parseBLF(const QString &filename, 
                        QVector<CANFrame> &frames, 
                        QString &errorMsg);
    
    /**
     * @brief 解析ASC文件
     */
    static bool parseASC(const QString &filename, 
                        QVector<CANFrame> &frames, 
                        QString &errorMsg);
    
    /**
     * @brief 解析CSV文件
     */
    static bool parseCSV(const QString &filename, 
                        QVector<CANFrame> &frames, 
                        QString &errorMsg);
    
private:
    /**
     * @brief 解析十六进制字符串
     */
    static bool parseHexString(const QString &str, quint32 &value);
    
    /**
     * @brief 解析数据字节
     */
    static bool parseDataBytes(const QString &str, QByteArray &data);
};

#endif // FRAMEFILEPARSER_H
