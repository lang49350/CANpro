/**
 * @file CANViewRealTimeSaver.h
 * @brief CAN数据实时保存类
 * @author CANAnalyzerPro Team
 * @date 2024-02-10
 */

#ifndef CANVIEWREALTIMESAVER_H
#define CANVIEWREALTIMESAVER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include "../core/CANFrame.h"

/**
 * @brief CAN数据实时保存类
 * 
 * 负责将接收到的CAN数据实时保存到文件
 */
class CANViewRealTimeSaver : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief 保存格式枚举
     */
    enum SaveFormat {
        FormatASC,      // Vector ASC格式
        FormatBLF,      // Vector BLF格式
        FormatCAN       // 通用CAN格式
    };
    
    /**
     * @brief 构造函数
     */
    explicit CANViewRealTimeSaver(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~CANViewRealTimeSaver();
    
    /**
     * @brief 开始实时保存
     * @param fileName 文件名
     * @param format 保存格式
     * @return 成功返回true
     */
    bool startSaving(const QString &fileName, SaveFormat format);
    
    /**
     * @brief 停止实时保存
     */
    void stopSaving();
    
    /**
     * @brief 是否正在保存
     */
    bool isSaving() const { return m_isSaving; }
    
    /**
     * @brief 批量保存多帧数据
     * @param frames CAN帧列表
     */
    void saveFrames(const QVector<CANFrame> &frames);
    
private:
    bool m_isSaving;
    SaveFormat m_format;
    QFile *m_file;
    QTextStream *m_stream;
    
    // 文件头尾
    void writeASCHeader();
    void writeASCFooter();
    void writeBLFHeader();
    void writeBLFFooter();
    void writeCANHeader();
    void writeCANFooter();
    
    // 格式化函数（返回字符串）
    QString formatASCFrame(const CANFrame &frame);
    QString formatBLFFrame(const CANFrame &frame);
    QString formatCANFrame(const CANFrame &frame);
};

#endif // CANVIEWREALTIMESAVER_H
