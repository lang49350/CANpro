/**
 * @file FilterHeaderWidget.h
 * @brief 表格筛选头部控件
 * @author CANAnalyzerPro Team
 * @date 2024-02-10
 */

#ifndef FILTERHEADERWIDGET_H
#define FILTERHEADERWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTableView>
#include <QHeaderView>
#include <QList>
#include "../core/CANFrameTableModel.h"

// 前向声明
class CustomComboBox;

/**
 * @brief 表格筛选头部控件
 * 
 * 功能：
 * - 在表格上方显示筛选控件行
 * - 自动与表格列对齐（像素级精确）
 * - 监听列宽变化，自动同步
 */
class FilterHeaderWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit FilterHeaderWidget(QTableView *tableView, QWidget *parent = nullptr);
    ~FilterHeaderWidget();
    
    // 获取筛选控件
    QLineEdit* getSeqEdit() const { return m_editSeq; }
    QLineEdit* getTimeEdit() const { return m_editTime; }
    QLineEdit* getIDEdit() const { return m_editID; }
    CustomComboBox* getTypeCombo() const { return m_comboType; }
    CustomComboBox* getDirCombo() const { return m_comboDir; }
    QLineEdit* getLenEdit() const { return m_editLen; }
    QLineEdit* getDataEdit() const { return m_editData; }
    QLineEdit* getChannelEdit() const { return m_editChannel; }
    
    // 更新列宽（初始化时调用）
    void updateColumnWidths();
    
private slots:
    void onColumnResized(int logicalIndex, int oldSize, int newSize);
    void onSectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);
    
private:
    void setupUi();
    QWidget* createFilterWidget(int column);
    
    QTableView *m_tableView;
    QHBoxLayout *m_layout;
    QList<QWidget*> m_filterWidgets;
    
    // 筛选控件
    QLineEdit *m_editSeq;
    QLineEdit *m_editTime;
    QLineEdit *m_editID;
    CustomComboBox *m_comboType;
    CustomComboBox *m_comboDir;
    QLineEdit *m_editLen;
    QLineEdit *m_editData;
    QLineEdit *m_editChannel;
};

#endif // FILTERHEADERWIDGET_H
