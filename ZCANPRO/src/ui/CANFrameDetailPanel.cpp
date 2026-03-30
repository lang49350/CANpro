/**
 * @file CANFrameDetailPanel.cpp
 * @brief CAN帧详情面板实现
 * @author CANAnalyzerPro Team
 * @date 2024-02-13
 */

#include "CANFrameDetailPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFont>
#include <QDateTime>

CANFrameDetailPanel::CANFrameDetailPanel(QWidget *parent)
    : QWidget(parent)
    , m_dataFormat(0)
    , m_idFormat(0)
{
    setupUi();
}

CANFrameDetailPanel::~CANFrameDetailPanel()
{
}

void CANFrameDetailPanel::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    
    // ==================== 基本信息组 ====================
    QGroupBox *basicGroup = new QGroupBox(QString::fromUtf8("基本信息"));
    QGridLayout *basicLayout = new QGridLayout(basicGroup);
    basicLayout->setContentsMargins(10, 10, 10, 10);
    basicLayout->setSpacing(8);
    
    // 创建标签（左侧）
    QLabel *lblSeq = new QLabel(QString::fromUtf8("序号:"));
    QLabel *lblTime = new QLabel(QString::fromUtf8("时间戳:"));
    QLabel *lblChannel = new QLabel(QString::fromUtf8("通道:"));
    QLabel *lblID = new QLabel(QString::fromUtf8("帧ID:"));
    QLabel *lblType = new QLabel(QString::fromUtf8("类型:"));
    QLabel *lblDir = new QLabel(QString::fromUtf8("方向:"));
    QLabel *lblLength = new QLabel(QString::fromUtf8("长度:"));
    
    // 使用控件继承字体，避免未设字号时出现 QFont::setPointSize(-1) 警告
    QFont labelFont = font();
    labelFont.setBold(true);
    lblSeq->setFont(labelFont);
    lblTime->setFont(labelFont);
    lblChannel->setFont(labelFont);
    lblID->setFont(labelFont);
    lblType->setFont(labelFont);
    lblDir->setFont(labelFont);
    lblLength->setFont(labelFont);
    
    // 创建值标签（右侧）
    m_lblSeqValue = new QLabel("-");
    m_lblTimeValue = new QLabel("-");
    m_lblChannelValue = new QLabel("-");
    m_lblIDValue = new QLabel("-");
    m_lblTypeValue = new QLabel("-");
    m_lblDirValue = new QLabel("-");
    m_lblLengthValue = new QLabel("-");
    
    // 设置值标签样式（等宽字体）
    QFont valueFont("Consolas", 10);
    m_lblSeqValue->setFont(valueFont);
    m_lblTimeValue->setFont(valueFont);
    m_lblChannelValue->setFont(valueFont);
    m_lblIDValue->setFont(valueFont);
    m_lblTypeValue->setFont(valueFont);
    m_lblDirValue->setFont(valueFont);
    m_lblLengthValue->setFont(valueFont);
    
    // 设置值标签可选择（方便复制）
    m_lblSeqValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lblTimeValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lblChannelValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lblIDValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lblTypeValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lblDirValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lblLengthValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    
    // 布局（2列）
    basicLayout->addWidget(lblSeq, 0, 0);
    basicLayout->addWidget(m_lblSeqValue, 0, 1);
    basicLayout->addWidget(lblTime, 1, 0);
    basicLayout->addWidget(m_lblTimeValue, 1, 1);
    basicLayout->addWidget(lblChannel, 2, 0);
    basicLayout->addWidget(m_lblChannelValue, 2, 1);
    basicLayout->addWidget(lblID, 3, 0);
    basicLayout->addWidget(m_lblIDValue, 3, 1);
    basicLayout->addWidget(lblType, 4, 0);
    basicLayout->addWidget(m_lblTypeValue, 4, 1);
    basicLayout->addWidget(lblDir, 5, 0);
    basicLayout->addWidget(m_lblDirValue, 5, 1);
    basicLayout->addWidget(lblLength, 6, 0);
    basicLayout->addWidget(m_lblLengthValue, 6, 1);
    
    basicLayout->setColumnStretch(1, 1);  // 右侧列拉伸
    
    mainLayout->addWidget(basicGroup);
    
    // ==================== CAN-FD信息组 ====================
    QGroupBox *fdGroup = new QGroupBox(QString::fromUtf8("CAN-FD信息"));
    QGridLayout *fdLayout = new QGridLayout(fdGroup);
    fdLayout->setContentsMargins(10, 10, 10, 10);
    fdLayout->setSpacing(8);
    
    QLabel *lblBRS = new QLabel(QString::fromUtf8("位速率切换(BRS):"));
    QLabel *lblESI = new QLabel(QString::fromUtf8("错误状态(ESI):"));
    lblBRS->setFont(labelFont);
    lblESI->setFont(labelFont);
    
    m_lblBRSValue = new QLabel("-");
    m_lblESIValue = new QLabel("-");
    m_lblBRSValue->setFont(valueFont);
    m_lblESIValue->setFont(valueFont);
    m_lblBRSValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lblESIValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    
    fdLayout->addWidget(lblBRS, 0, 0);
    fdLayout->addWidget(m_lblBRSValue, 0, 1);
    fdLayout->addWidget(lblESI, 1, 0);
    fdLayout->addWidget(m_lblESIValue, 1, 1);
    fdLayout->setColumnStretch(1, 1);
    
    mainLayout->addWidget(fdGroup);
    
    // ==================== 数据显示组 ====================
    QGroupBox *dataGroup = new QGroupBox(QString::fromUtf8("数据内容"));
    QVBoxLayout *dataLayout = new QVBoxLayout(dataGroup);
    dataLayout->setContentsMargins(10, 10, 10, 10);
    dataLayout->setSpacing(5);
    
    // 十六进制视图
    QLabel *lblHex = new QLabel(QString::fromUtf8("十六进制:"));
    lblHex->setFont(labelFont);
    m_txtDataHex = new QTextEdit();
    m_txtDataHex->setReadOnly(true);
    m_txtDataHex->setFont(QFont("Consolas", 10));
    m_txtDataHex->setMinimumHeight(80);
    m_txtDataHex->setLineWrapMode(QTextEdit::WidgetWidth);
    
    // ASCII视图
    QLabel *lblAscii = new QLabel(QString::fromUtf8("ASCII:"));
    lblAscii->setFont(labelFont);
    m_txtDataAscii = new QTextEdit();
    m_txtDataAscii->setReadOnly(true);
    m_txtDataAscii->setFont(QFont("Consolas", 10));
    m_txtDataAscii->setMinimumHeight(60);
    m_txtDataAscii->setLineWrapMode(QTextEdit::WidgetWidth);
    
    dataLayout->addWidget(lblHex);
    dataLayout->addWidget(m_txtDataHex);
    dataLayout->addWidget(lblAscii);
    dataLayout->addWidget(m_txtDataAscii);
    
    mainLayout->addWidget(dataGroup);
    mainLayout->addStretch();
    
    // 设置整体样式
    setStyleSheet(
        "QGroupBox { "
        "   font-weight: bold; "
        "   border: 1px solid #CCCCCC; "
        "   border-radius: 5px; "
        "   margin-top: 10px; "
        "   padding-top: 10px; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   subcontrol-position: top left; "
        "   padding: 0 5px; "
        "   color: #1976D2; "
        "}"
    );
}

void CANFrameDetailPanel::showFrame(const CANFrame &frame, quint64 sequenceNumber)
{
    // 序号
    m_lblSeqValue->setText(QString::number(sequenceNumber));
    
    // 时间戳（微秒转秒）
    double seconds = frame.timestamp / 1000000.0;
    m_lblTimeValue->setText(QString::number(seconds, 'f', 6) + " s");
    
    // 通道
    m_lblChannelValue->setText(QString("CAN%1").arg(frame.channel));
    
    // 帧ID
    if (m_idFormat == 0) {
        // 十六进制
        m_lblIDValue->setText(QString("0x%1").arg(frame.id, 0, 16, QChar('0')).toUpper());
    } else {
        // 十进制
        m_lblIDValue->setText(QString::number(frame.id));
    }
    
    // 类型
    QString type;
    if (frame.isCanFD) {
        type = "CAN-FD";
    } else if (frame.isExtended) {
        type = QString::fromUtf8("扩展帧");
    } else {
        type = QString::fromUtf8("标准帧");
    }
    if (frame.isRemote) {
        type += QString::fromUtf8(" (远程帧)");
    }
    m_lblTypeValue->setText(type);
    
    // 方向
    if (frame.isReceived) {
        m_lblDirValue->setText(QString::fromUtf8("接收 (Rx)"));
        m_lblDirValue->setStyleSheet("color: #2E7D32;");  // 深绿色
    } else {
        m_lblDirValue->setText(QString::fromUtf8("发送 (Tx)"));
        m_lblDirValue->setStyleSheet("color: #1565C0;");  // 深蓝色
    }
    
    // 长度
    m_lblLengthValue->setText(QString("%1 字节").arg(frame.length));
    
    // CAN-FD特有信息
    if (frame.isCanFD) {
        m_lblBRSValue->setText(frame.isBRS ? QString::fromUtf8("是") : QString::fromUtf8("否"));
        m_lblESIValue->setText(frame.isESI ? QString::fromUtf8("错误") : QString::fromUtf8("正常"));
    } else {
        m_lblBRSValue->setText("-");
        m_lblESIValue->setText("-");
    }
    
    // 数据显示
    switch (m_dataFormat) {
        case 0:
            // 十六进制
            m_txtDataHex->setText(formatDataHex(frame.data));
            break;
        case 1:
            // 十进制
            m_txtDataHex->setText(formatDataDec(frame.data));
            break;
        case 2:
            // 二进制
            m_txtDataHex->setText(formatDataBin(frame.data));
            break;
        case 3:
            // ASCII (已经在下面单独显示)
            m_txtDataHex->setText(formatDataAscii(frame.data));
            break;
        default:
            m_txtDataHex->setText(formatDataHex(frame.data));
            break;
    }
    
    // ASCII显示
    m_txtDataAscii->setText(formatDataAscii(frame.data));
}

void CANFrameDetailPanel::clear()
{
    m_lblSeqValue->setText("-");
    m_lblTimeValue->setText("-");
    m_lblChannelValue->setText("-");
    m_lblIDValue->setText("-");
    m_lblTypeValue->setText("-");
    m_lblDirValue->setText("-");
    m_lblDirValue->setStyleSheet("");
    m_lblLengthValue->setText("-");
    m_lblBRSValue->setText("-");
    m_lblESIValue->setText("-");
    m_txtDataHex->clear();
    m_txtDataAscii->clear();
}

void CANFrameDetailPanel::setDataFormat(int format)
{
    m_dataFormat = format;
}

void CANFrameDetailPanel::setIDFormat(int format)
{
    m_idFormat = format;
}

QString CANFrameDetailPanel::formatDataHex(const QByteArray &data) const
{
    QString result;
    
    // 每行显示16字节
    for (int i = 0; i < data.size(); i++) {
        if (i > 0 && i % 16 == 0) {
            result += "\n";
        } else if (i > 0) {
            result += " ";
        }
        
        result += QString("%1").arg((quint8)data[i], 2, 16, QChar('0')).toUpper();
    }
    
    return result;
}

QString CANFrameDetailPanel::formatDataAscii(const QByteArray &data) const
{
    QString result;
    
    // 每行显示16字节
    for (int i = 0; i < data.size(); i++) {
        if (i > 0 && i % 16 == 0) {
            result += "\n";
        }
        
        quint8 byte = (quint8)data[i];
        if (byte >= 32 && byte <= 126) {
            // 可打印字符
            result += QChar(byte);
        } else {
            // 不可打印字符显示为点
            result += ".";
        }
    }
    
    return result;
}

QString CANFrameDetailPanel::formatDataDec(const QByteArray &data) const
{
    QString result;
    
    // 每行显示8字节（十进制占用空间大）
    for (int i = 0; i < data.size(); i++) {
        if (i > 0 && i % 8 == 0) {
            result += "\n";
        } else if (i > 0) {
            result += " ";
        }
        
        result += QString("%1").arg((quint8)data[i], 3, 10, QChar(' '));
    }
    
    return result;
}

QString CANFrameDetailPanel::formatDataBin(const QByteArray &data) const
{
    QString result;
    
    // 每行显示4字节（二进制占用空间很大）
    for (int i = 0; i < data.size(); i++) {
        if (i > 0 && i % 4 == 0) {
            result += "\n";
        } else if (i > 0) {
            result += " ";
        }
        
        quint8 byte = (quint8)data[i];
        // 转换为8位二进制字符串
        for (int bit = 7; bit >= 0; bit--) {
            result += (byte & (1 << bit)) ? '1' : '0';
        }
    }
    
    return result;
}
