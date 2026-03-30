/**
 * @file FilterHeaderWidget.cpp
 * @brief 表格筛选头部控件实现
 */

#include "FilterHeaderWidget.h"
#include "CustomComboBox.h"
#include <QRegularExpressionValidator>

FilterHeaderWidget::FilterHeaderWidget(QTableView *tableView, QWidget *parent)
    : QWidget(parent)
    , m_tableView(tableView)
    , m_editSeq(nullptr)
    , m_editTime(nullptr)
    , m_editID(nullptr)
    , m_comboType(nullptr)
    , m_comboDir(nullptr)
    , m_editLen(nullptr)
    , m_editData(nullptr)
    , m_editChannel(nullptr)
{
    setupUi();
    
    // 监听表格列宽变化
    connect(m_tableView->horizontalHeader(), &QHeaderView::sectionResized,
            this, &FilterHeaderWidget::onColumnResized);
    
    // 监听列移动
    connect(m_tableView->horizontalHeader(), &QHeaderView::sectionMoved,
            this, &FilterHeaderWidget::onSectionMoved);
}

FilterHeaderWidget::~FilterHeaderWidget()
{
}

void FilterHeaderWidget::setupUi()
{
    // 创建水平布局
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    
    // 为每一列创建筛选控件
    for (int col = 0; col < 8; ++col) {  // 8列
        QWidget *filterWidget = createFilterWidget(col);
        m_filterWidgets.append(filterWidget);
        m_layout->addWidget(filterWidget);
    }
    
    setFixedHeight(35);
}

QWidget* FilterHeaderWidget::createFilterWidget(int column)
{
    QWidget *container = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);
    
    QWidget *widget = nullptr;
    
    switch (column) {
        case CANFrameTableModel::COL_SEQ:
            m_editSeq = new QLineEdit();
            m_editSeq->setPlaceholderText(QString::fromUtf8("序号"));
            widget = m_editSeq;
            break;
            
        case CANFrameTableModel::COL_TIME:
            m_editTime = new QLineEdit();
            m_editTime->setPlaceholderText(QString::fromUtf8("时间筛选"));
            widget = m_editTime;
            break;
            
        case CANFrameTableModel::COL_CHANNEL:
            m_editChannel = new QLineEdit();
            m_editChannel->setPlaceholderText(QString::fromUtf8("通道"));
            widget = m_editChannel;
            break;
            
        case CANFrameTableModel::COL_ID:
            m_editID = new QLineEdit();
            m_editID->setPlaceholderText(QString::fromUtf8("ID筛选"));
            widget = m_editID;
            break;
            
        case CANFrameTableModel::COL_TYPE:
            m_comboType = new CustomComboBox(this, true);
            m_comboType->addItem(QString::fromUtf8("全部"));
            m_comboType->addItem(QString::fromUtf8("标准帧"));
            m_comboType->addItem(QString::fromUtf8("CAN"));
            m_comboType->addItem(QString::fromUtf8("CANFD"));
            m_comboType->addItem(QString::fromUtf8("CANFD加速"));
            m_comboType->setMinimumWidth(80);
            // 设置下拉框样式，确保文字不被截断
            m_comboType->setStyleSheet(
                "QComboBox { "
                "   padding-left: 5px; "
                "   padding-right: 20px; "  // 为箭头留出空间
                "}"
            );
            widget = m_comboType;
            break;
            
        case CANFrameTableModel::COL_DIR:
            m_comboDir = new CustomComboBox(this, true);
            m_comboDir->addItem(QString::fromUtf8("全部"));
            m_comboDir->addItem(QString::fromUtf8("Tx"));
            m_comboDir->addItem(QString::fromUtf8("Rx"));
            m_comboDir->setMinimumWidth(70);  // 确保下拉框宽度足够
            // 设置下拉框样式，确保文字不被截断
            m_comboDir->setStyleSheet(
                "QComboBox { "
                "   padding-left: 5px; "
                "   padding-right: 20px; "  // 为箭头留出空间
                "}"
            );
            widget = m_comboDir;
            break;
            
        case CANFrameTableModel::COL_LEN:
            m_editLen = new QLineEdit();
            m_editLen->setPlaceholderText(QString::fromUtf8("长度"));
            widget = m_editLen;
            break;
            
        case CANFrameTableModel::COL_DATA: {
            m_editData = new QLineEdit();
            m_editData->setPlaceholderText(QString::fromUtf8("数据筛选(HEX)"));
            QRegularExpressionValidator *dataValidator = 
                new QRegularExpressionValidator(QRegularExpression("^[0-9A-Fa-f ]*$"), m_editData);
            m_editData->setValidator(dataValidator);
            widget = m_editData;
            break;
        }
    }
    
    if (widget) {
        layout->addWidget(widget);
    }
    
    return container;
}

void FilterHeaderWidget::updateColumnWidths()
{
    for (int col = 0; col < m_filterWidgets.size(); ++col) {
        int width = m_tableView->columnWidth(col);
        m_filterWidgets[col]->setFixedWidth(width);
    }
}

void FilterHeaderWidget::onColumnResized(int logicalIndex, int oldSize, int newSize)
{
    Q_UNUSED(oldSize);
    
    if (logicalIndex < m_filterWidgets.size()) {
        m_filterWidgets[logicalIndex]->setFixedWidth(newSize);
    }
}

void FilterHeaderWidget::onSectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex)
{
    Q_UNUSED(logicalIndex);
    Q_UNUSED(oldVisualIndex);
    Q_UNUSED(newVisualIndex);
    
    // 列移动时重新布局
    updateColumnWidths();
}
