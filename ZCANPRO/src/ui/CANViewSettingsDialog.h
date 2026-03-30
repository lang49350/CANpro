/**
 * @file CANViewSettingsDialog.h
 * @brief CAN视图设置对话框
 * @author CANAnalyzerPro Team
 * @date 2024-02-09
 */

#ifndef CANVIEWSETTINGSDIALOG_H
#define CANVIEWSETTINGSDIALOG_H

#include <QDialog>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QLineEdit>
#include <QCheckBox>
#include "../core/IDFilterRule.h"

// 前向声明
class CustomComboBox;

/**
 * @brief CAN视图设置对话框
 * 
 * 功能：
 * - 显示设置：时间格式、数据格式、帧ID格式、最大帧数
 * - 过滤设置：帧ID过滤规则
 */
class CANViewSettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    /**
     * @brief 时间格式枚举
     */
    enum TimeFormat {
        RelativeTime = 0,  // 相对时间(秒)
        AbsoluteTime = 1   // 绝对时间
    };
    
    /**
     * @brief 数据格式枚举
     */
    enum DataFormat {
        Hexadecimal = 0,   // 十六进制
        Decimal = 1,       // 十进制
        Binary = 2,        // 二进制
        ASCII = 3          // ASCII码
    };
    
    explicit CANViewSettingsDialog(QWidget *parent = nullptr);
    ~CANViewSettingsDialog();
    
    // 获取显示设置值
    TimeFormat getTimeFormat() const;
    DataFormat getDataFormat() const;
    DataFormat getIDFormat() const;
    
    // 获取列显示状态
    bool isColumnVisible(int column) const;
    QMap<int, bool> getColumnVisibility() const;
    void setColumnVisibility(const QMap<int, bool> &visibility);
    
    // 设置显示当前值
    void setTimeFormat(TimeFormat format);
    void setDataFormat(DataFormat format);
    void setIDFormat(DataFormat format);
    
    // 获取/设置过滤规则
    IDFilterRule getFilterRule() const;
    void setFilterRule(const IDFilterRule &rule);
    
signals:
    void settingsChanged();  // 设置改变信号
    
private slots:
    void onConfirmClicked();
    void onCancelClicked();
    void onFilterModeChanged();
    
private:
    void setupUi();
    QWidget* createDisplayTab();
    QWidget* createFilterTab();
    void loadSettings();
    void saveSettings();
    void updateFilterNoteText();
    
    // 显示设置UI组件
    CustomComboBox *m_comboTimeFormat;    // 时间格式
    CustomComboBox *m_comboDataFormat;    // 数据格式
    CustomComboBox *m_comboIDFormat;      // 帧ID格式
    QSlider *m_sliderMaxFrames;      // 最大帧数滑块
    QLabel *m_lblMaxFramesValue;     // 最大帧数显示
    
    // 列显示项复选框
    QCheckBox *m_chkShowSeq;         // 显示序号
    QCheckBox *m_chkShowTime;        // 显示时间标识
    QCheckBox *m_chkShowChannel;     // 显示源通道
    QCheckBox *m_chkShowID;          // 显示帧ID
    QCheckBox *m_chkShowType;        // 显示CAN类型
    QCheckBox *m_chkShowDir;         // 显示方向
    QCheckBox *m_chkShowLen;         // 显示长度
    QCheckBox *m_chkShowData;        // 显示数据
    
    // 过滤设置UI组件
    QCheckBox *m_chkEnableFilter;    // 启用过滤
    QRadioButton *m_rbFilterOnReceive;  // 接收时过滤
    QRadioButton *m_rbFilterOnDisplay;  // 显示时过滤
    QRadioButton *m_rbHex;           // 数字(Hex)
    QRadioButton *m_rbString;        // 字符串
    CustomComboBox *m_comboOp1;           // 第一个运算符
    QLineEdit *m_editValue1;         // 第一个值
    QRadioButton *m_rbAnd;           // 与
    QRadioButton *m_rbOr;            // 或
    CustomComboBox *m_comboOp2;           // 第二个运算符
    QLineEdit *m_editValue2;         // 第二个值
    QLabel *m_lblFilterNote;         // 过滤注意事项
    
    // 当前值
    TimeFormat m_timeFormat;
    DataFormat m_dataFormat;
    DataFormat m_idFormat;
    IDFilterRule m_filterRule;
};

#endif // CANVIEWSETTINGSDIALOG_H
