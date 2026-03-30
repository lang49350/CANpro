/**
 * @file CANFrameDetailPanel.h
 * @brief CAN帧详情面板 - 显示选中帧的完整信息
 * @author CANAnalyzerPro Team
 * @date 2024-02-13
 */

#ifndef CANFRAMEDETAILPANEL_H
#define CANFRAMEDETAILPANEL_H

#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QGroupBox>
#include <QGridLayout>
#include "../core/CANFrame.h"

/**
 * @brief CAN帧详情面板
 * 
 * 功能：
 * - 显示选中CAN帧的完整信息
 * - 支持CAN-FD大数据包显示（最多64字节）
 * - 十六进制和ASCII双视图
 * - 详细的帧属性信息
 */
class CANFrameDetailPanel : public QWidget
{
    Q_OBJECT
    
public:
    explicit CANFrameDetailPanel(QWidget *parent = nullptr);
    ~CANFrameDetailPanel();
    
    /**
     * @brief 显示CAN帧详情
     * @param frame CAN帧数据
     * @param sequenceNumber 序号
     */
    void showFrame(const CANFrame &frame, quint64 sequenceNumber);
    
    /**
     * @brief 清空显示
     */
    void clear();
    
    /**
     * @brief 设置数据显示格式
     * @param format 0=十六进制, 1=十进制
     */
    void setDataFormat(int format);
    
    /**
     * @brief 设置ID显示格式
     * @param format 0=十六进制, 1=十进制
     */
    void setIDFormat(int format);
    
private:
    void setupUi();
    QString formatDataHex(const QByteArray &data) const;
    QString formatDataAscii(const QByteArray &data) const;
    QString formatDataDec(const QByteArray &data) const;
    QString formatDataBin(const QByteArray &data) const;
    
    // 基本信息标签
    QLabel *m_lblSeqValue;        // 序号
    QLabel *m_lblTimeValue;       // 时间戳
    QLabel *m_lblChannelValue;    // 通道
    QLabel *m_lblIDValue;         // 帧ID
    QLabel *m_lblTypeValue;       // 帧类型
    QLabel *m_lblDirValue;        // 方向
    QLabel *m_lblLengthValue;     // 数据长度
    
    // CAN-FD特有信息
    QLabel *m_lblBRSValue;        // 位速率切换
    QLabel *m_lblESIValue;        // 错误状态指示
    
    // 数据显示区
    QTextEdit *m_txtDataHex;      // 十六进制数据
    QTextEdit *m_txtDataAscii;    // ASCII数据
    
    // 显示格式
    int m_dataFormat;             // 0=十六进制, 1=十进制
    int m_idFormat;               // 0=十六进制, 1=十进制
};

#endif // CANFRAMEDETAILPANEL_H
