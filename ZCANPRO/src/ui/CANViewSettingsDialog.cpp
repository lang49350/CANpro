/**
 * @file CANViewSettingsDialog.cpp
 * @brief CAN视图设置对话框实现
 * @author CANAnalyzerPro Team
 * @date 2024-02-09
 */

#include "CANViewSettingsDialog.h"
#include "CustomComboBox.h"
#include "StyleManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QScrollArea>
#include <QButtonGroup>
#include <QSettings>
#include <QDebug>

CANViewSettingsDialog::CANViewSettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_timeFormat(RelativeTime)
    , m_dataFormat(Hexadecimal)
    , m_idFormat(Hexadecimal)
{
    setupUi();
    loadSettings();
    
    qDebug() << "✅ CANViewSettingsDialog 创建";
}

CANViewSettingsDialog::~CANViewSettingsDialog()
{
}

void CANViewSettingsDialog::setupUi()
{
    setWindowTitle("设置");
    setMinimumSize(600, 600);  // 增加高度到600，确保不需要滚动
    
    // 设置无边框对话框样式，添加边框和圆角
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);  // 确保样式应用到背景
    
    // 使用对象名称选择器来应用样式
    setObjectName("CANViewSettingsDialog");
    setStyleSheet(
        "QDialog#CANViewSettingsDialog { "
        "   background-color: white; "
        "   border: 2px solid #607D8B; "
        "   border-radius: 5px; "
        "}"
    );
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(2, 2, 2, 2);  // 留出边框空间
    
    // 1. 标题栏（蓝灰色）
    QWidget *titleBar = new QWidget();
    titleBar->setMinimumHeight(40);
    titleBar->setMaximumHeight(40);
    titleBar->setStyleSheet("QWidget { background-color: #607D8B; }");
    
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 5, 0);
    
    QLabel *lblTitle = new QLabel("设置");
    lblTitle->setStyleSheet("color: white; font-size: 14px; font-weight: bold;");
    titleLayout->addWidget(lblTitle);
    titleLayout->addStretch();
    
    // 关闭按钮
    QPushButton *btnClose = new QPushButton("✕");
    btnClose->setStyleSheet(
        "QPushButton { "
        "   background-color: transparent; "
        "   color: white; "
        "   border: none; "
        "   min-width: 40px; max-width: 40px; "
        "   min-height: 40px; max-height: 40px; "
        "   font-size: 16px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #E81123; "
        "}"
    );
    connect(btnClose, &QPushButton::clicked, this, &CANViewSettingsDialog::onCancelClicked);
    titleLayout->addWidget(btnClose);
    
    mainLayout->addWidget(titleBar);
    
    // 2. 标签页（白色背景）
    QTabWidget *tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(
        "QTabWidget::pane { "
        "   border: none; "
        "   background-color: white; "
        "} "
        "QTabBar::tab { "
        "   background-color: #E0E0E0; "
        "   color: black; "
        "   padding: 8px 20px; "
        "   margin-right: 2px; "
        "} "
        "QTabBar::tab:selected { "
        "   background-color: #607D8B; "
        "   color: white; "
        "}"
    );
    
    // 添加显示设置标签页
    QWidget *displayTab = createDisplayTab();
    tabWidget->addTab(displayTab, "显示设置");
    
    // 添加过滤设置标签页
    QWidget *filterTab = createFilterTab();
    tabWidget->addTab(filterTab, "过滤设置");
    
    mainLayout->addWidget(tabWidget);
    
    // 3. 底部按钮栏（浅灰色背景）
    QWidget *buttonBar = new QWidget();
    buttonBar->setStyleSheet("QWidget { background-color: #ECEFF1; }");
    buttonBar->setMinimumHeight(60);
    buttonBar->setMaximumHeight(60);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonBar);
    buttonLayout->setContentsMargins(20, 12, 20, 12);  // 增加上下边距
    buttonLayout->addStretch();
    
    QPushButton *btnConfirm = new QPushButton("确定");
    btnConfirm->setMinimumSize(100, 35);  // 和设备管理对话框一致
    
    QString btnStyle = 
        "QPushButton { "
        "   background-color: #E3F2FD; "
        "   color: black; "
        "   border: 1px solid #90CAF9; "
        "   padding: 5px 15px; "
        "   font-size: 12px; "
        "   border-radius: 3px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #BBDEFB; "
        "   border: 1px solid #64B5F6; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #90CAF9; "
        "}";
    
    btnConfirm->setStyleSheet(btnStyle);
    
    connect(btnConfirm, &QPushButton::clicked, this, &CANViewSettingsDialog::onConfirmClicked);
    
    buttonLayout->addWidget(btnConfirm);
    
    mainLayout->addWidget(buttonBar);
}

void CANViewSettingsDialog::loadSettings()
{
    QSettings settings("CANTools", "CANAnalyzerPro");
    
    // 加载时间格式
    m_timeFormat = static_cast<TimeFormat>(
        settings.value("CANView/TimeFormat", RelativeTime).toInt());
    m_comboTimeFormat->setCurrentIndex(m_timeFormat);
    
    // 加载数据格式
    m_dataFormat = static_cast<DataFormat>(
        settings.value("CANView/DataFormat", Hexadecimal).toInt());
    m_comboDataFormat->setCurrentIndex(m_dataFormat);
    
    // 加载帧ID格式
    m_idFormat = static_cast<DataFormat>(
        settings.value("CANView/IDFormat", Hexadecimal).toInt());
    m_comboIDFormat->setCurrentIndex(m_idFormat);
    
    // 加载列显示状态（默认全部显示）
    m_chkShowSeq->setChecked(settings.value("CANView/ShowSeq", true).toBool());
    m_chkShowTime->setChecked(settings.value("CANView/ShowTime", true).toBool());
    m_chkShowChannel->setChecked(settings.value("CANView/ShowChannel", true).toBool());
    m_chkShowID->setChecked(settings.value("CANView/ShowID", true).toBool());
    m_chkShowType->setChecked(settings.value("CANView/ShowType", true).toBool());
    m_chkShowDir->setChecked(settings.value("CANView/ShowDir", true).toBool());
    m_chkShowLen->setChecked(settings.value("CANView/ShowLen", true).toBool());
    m_chkShowData->setChecked(settings.value("CANView/ShowData", true).toBool());
    
    // 加载过滤规则
    settings.beginGroup("IDFilter");
    m_filterRule.enabled = settings.value("enabled", false).toBool();
    m_filterRule.mode = static_cast<IDFilterRule::FilterMode>(settings.value("mode", IDFilterRule::FilterOnDisplay).toInt());
    m_filterRule.isHexMode = settings.value("isHexMode", true).toBool();
    m_filterRule.compareOp1 = static_cast<IDFilterRule::CompareOp>(settings.value("compareOp1", 0).toInt());
    m_filterRule.value1 = settings.value("value1", "").toString();
    m_filterRule.logicOp = static_cast<IDFilterRule::LogicOp>(settings.value("logicOp", 1).toInt());
    m_filterRule.compareOp2 = static_cast<IDFilterRule::CompareOp>(settings.value("compareOp2", 0).toInt());
    m_filterRule.value2 = settings.value("value2", "").toString();
    settings.endGroup();
    
    // 应用过滤规则到UI
    setFilterRule(m_filterRule);
    
    qDebug() << "📖 加载设置: 时间格式=" << m_timeFormat 
             << ", 数据格式=" << m_dataFormat 
             << ", ID格式=" << m_idFormat
             << ", 过滤启用=" << m_filterRule.enabled
             << ", 过滤模式=" << m_filterRule.mode;
}

void CANViewSettingsDialog::saveSettings()
{
    QSettings settings("CANTools", "CANAnalyzerPro");
    
    settings.setValue("CANView/TimeFormat", m_comboTimeFormat->currentIndex());
    settings.setValue("CANView/DataFormat", m_comboDataFormat->currentIndex());
    settings.setValue("CANView/IDFormat", m_comboIDFormat->currentIndex());
    
    // 保存列显示状态
    settings.setValue("CANView/ShowSeq", m_chkShowSeq->isChecked());
    settings.setValue("CANView/ShowTime", m_chkShowTime->isChecked());
    settings.setValue("CANView/ShowChannel", m_chkShowChannel->isChecked());
    settings.setValue("CANView/ShowID", m_chkShowID->isChecked());
    settings.setValue("CANView/ShowType", m_chkShowType->isChecked());
    settings.setValue("CANView/ShowDir", m_chkShowDir->isChecked());
    settings.setValue("CANView/ShowLen", m_chkShowLen->isChecked());
    settings.setValue("CANView/ShowData", m_chkShowData->isChecked());
    
    // 保存过滤规则
    IDFilterRule rule = getFilterRule();
    settings.beginGroup("IDFilter");
    settings.setValue("enabled", rule.enabled);
    settings.setValue("mode", rule.mode);
    settings.setValue("isHexMode", rule.isHexMode);
    settings.setValue("compareOp1", rule.compareOp1);
    settings.setValue("value1", rule.value1);
    settings.setValue("logicOp", rule.logicOp);
    settings.setValue("compareOp2", rule.compareOp2);
    settings.setValue("value2", rule.value2);
    settings.endGroup();
    
    qDebug() << "💾 保存设置: 时间格式=" << m_comboTimeFormat->currentIndex()
             << ", 数据格式=" << m_comboDataFormat->currentIndex()
             << ", ID格式=" << m_comboIDFormat->currentIndex()
             << ", 过滤启用=" << rule.enabled
             << ", 过滤模式=" << rule.mode;
}

// ==================== Getters ====================

CANViewSettingsDialog::TimeFormat CANViewSettingsDialog::getTimeFormat() const
{
    return static_cast<TimeFormat>(m_comboTimeFormat->currentIndex());
}

CANViewSettingsDialog::DataFormat CANViewSettingsDialog::getDataFormat() const
{
    return static_cast<DataFormat>(m_comboDataFormat->currentIndex());
}

CANViewSettingsDialog::DataFormat CANViewSettingsDialog::getIDFormat() const
{
    return static_cast<DataFormat>(m_comboIDFormat->currentIndex());
}

bool CANViewSettingsDialog::isColumnVisible(int column) const
{
    switch (column) {
        case 0: return m_chkShowSeq->isChecked();      // COL_SEQ
        case 1: return m_chkShowTime->isChecked();     // COL_TIME
        case 2: return m_chkShowChannel->isChecked();  // COL_CHANNEL
        case 3: return m_chkShowID->isChecked();       // COL_ID
        case 4: return m_chkShowType->isChecked();     // COL_TYPE
        case 5: return m_chkShowDir->isChecked();      // COL_DIR
        case 6: return m_chkShowLen->isChecked();      // COL_LEN
        case 7: return m_chkShowData->isChecked();     // COL_DATA
        default: return true;
    }
}

QMap<int, bool> CANViewSettingsDialog::getColumnVisibility() const
{
    QMap<int, bool> visibility;
    visibility[0] = m_chkShowSeq->isChecked();
    visibility[1] = m_chkShowTime->isChecked();
    visibility[2] = m_chkShowChannel->isChecked();
    visibility[3] = m_chkShowID->isChecked();
    visibility[4] = m_chkShowType->isChecked();
    visibility[5] = m_chkShowDir->isChecked();
    visibility[6] = m_chkShowLen->isChecked();
    visibility[7] = m_chkShowData->isChecked();
    return visibility;
}

void CANViewSettingsDialog::setColumnVisibility(const QMap<int, bool> &visibility)
{
    if (visibility.contains(0)) m_chkShowSeq->setChecked(visibility[0]);
    if (visibility.contains(1)) m_chkShowTime->setChecked(visibility[1]);
    if (visibility.contains(2)) m_chkShowChannel->setChecked(visibility[2]);
    if (visibility.contains(3)) m_chkShowID->setChecked(visibility[3]);
    if (visibility.contains(4)) m_chkShowType->setChecked(visibility[4]);
    if (visibility.contains(5)) m_chkShowDir->setChecked(visibility[5]);
    if (visibility.contains(6)) m_chkShowLen->setChecked(visibility[6]);
    if (visibility.contains(7)) m_chkShowData->setChecked(visibility[7]);
}

// ==================== Setters ====================

void CANViewSettingsDialog::setTimeFormat(TimeFormat format)
{
    m_comboTimeFormat->setCurrentIndex(format);
}

void CANViewSettingsDialog::setDataFormat(DataFormat format)
{
    m_comboDataFormat->setCurrentIndex(format);
}

void CANViewSettingsDialog::setIDFormat(DataFormat format)
{
    m_comboIDFormat->setCurrentIndex(format);
}

// ==================== Slots ====================

void CANViewSettingsDialog::onConfirmClicked()
{
    saveSettings();
    emit settingsChanged();
    accept();
}

void CANViewSettingsDialog::onCancelClicked()
{
    reject();
}

QWidget* CANViewSettingsDialog::createDisplayTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);
    
    // 格式设置组
    QGroupBox *formatGroup = new QGroupBox("格式设置");
    formatGroup->setStyleSheet(StyleManager::groupBoxStyle());
    
    QGridLayout *formatLayout = new QGridLayout(formatGroup);
    formatLayout->setSpacing(10);
    formatLayout->setContentsMargins(15, 20, 15, 15);
    
    // 时间格式
    QLabel *lblTimeFormat = new QLabel("时间格式:");
    m_comboTimeFormat = new CustomComboBox();
    m_comboTimeFormat->addItem("相对时间(秒s)");
    m_comboTimeFormat->addItem("绝对时间");
    m_comboTimeFormat->setMinimumWidth(200);
    
    // 帧ID格式
    QLabel *lblIDFormat = new QLabel("帧ID格式:");
    m_comboIDFormat = new CustomComboBox();
    m_comboIDFormat->addItem("十六进制");
    m_comboIDFormat->addItem("十进制");
    m_comboIDFormat->addItem("二进制");
    m_comboIDFormat->setMinimumWidth(200);
    
    // 数据格式
    QLabel *lblDataFormat = new QLabel("数据格式:");
    m_comboDataFormat = new CustomComboBox();
    m_comboDataFormat->addItem("十六进制");
    m_comboDataFormat->addItem("十进制");
    m_comboDataFormat->addItem("二进制");
    m_comboDataFormat->addItem("ASCII码");
    m_comboDataFormat->setMinimumWidth(200);
    
    formatLayout->addWidget(lblTimeFormat, 0, 0);
    formatLayout->addWidget(m_comboTimeFormat, 0, 1);
    formatLayout->addWidget(lblIDFormat, 0, 2);
    formatLayout->addWidget(m_comboIDFormat, 0, 3);
    formatLayout->addWidget(lblDataFormat, 1, 0);
    formatLayout->addWidget(m_comboDataFormat, 1, 1);
    
    layout->addWidget(formatGroup);
    
    // 列表显示项设置组
    QGroupBox *columnGroup = new QGroupBox("列表显示项:");
    columnGroup->setStyleSheet(formatGroup->styleSheet());
    
    QGridLayout *columnLayout = new QGridLayout(columnGroup);
    columnLayout->setSpacing(10);
    columnLayout->setContentsMargins(15, 20, 15, 15);
    
    // 创建复选框（3列布局）
    m_chkShowSeq = new QCheckBox("序号");
    m_chkShowTime = new QCheckBox("时间标识");
    m_chkShowChannel = new QCheckBox("源通道");
    m_chkShowID = new QCheckBox("帧ID");
    m_chkShowType = new QCheckBox("CAN类型");
    m_chkShowDir = new QCheckBox("方向");
    m_chkShowLen = new QCheckBox("长度");
    m_chkShowData = new QCheckBox("数据");
    
    // 默认全部选中
    m_chkShowSeq->setChecked(true);
    m_chkShowTime->setChecked(true);
    m_chkShowChannel->setChecked(true);
    m_chkShowID->setChecked(true);
    m_chkShowType->setChecked(true);
    m_chkShowDir->setChecked(true);
    m_chkShowLen->setChecked(true);
    m_chkShowData->setChecked(true);
    
    // 布局（3列 x 3行）
    columnLayout->addWidget(m_chkShowSeq, 0, 0);
    columnLayout->addWidget(m_chkShowTime, 0, 1);
    columnLayout->addWidget(m_chkShowChannel, 0, 2);
    columnLayout->addWidget(m_chkShowID, 1, 0);
    columnLayout->addWidget(m_chkShowType, 1, 1);
    columnLayout->addWidget(m_chkShowDir, 1, 2);
    columnLayout->addWidget(m_chkShowLen, 2, 0);
    columnLayout->addWidget(m_chkShowData, 2, 1);
    
    layout->addWidget(columnGroup);
    layout->addStretch();
    
    return widget;
}

QWidget* CANViewSettingsDialog::createFilterTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(widget);
    mainLayout->setSpacing(6);  // 进一步减少间距
    mainLayout->setContentsMargins(12, 12, 12, 12);  // 进一步减少边距
    
    // 启用过滤复选框 - 放在最顶部，不在滚动区域内
    m_chkEnableFilter = new QCheckBox("启用帧ID过滤");
    m_chkEnableFilter->setStyleSheet(
        "QCheckBox { "
        "   font-weight: bold; "
        "   font-size: 10pt; "  // 从11pt减小到10pt
        "   padding: 6px; "  // 从8px减小到6px
        "   spacing: 8px; "
        "   background-color: #E3F2FD; "
        "   border: 1px solid #90CAF9; "
        "   border-radius: 4px; "
        "}"
        "QCheckBox:hover { "
        "   background-color: #BBDEFB; "
        "}"
    );
    m_chkEnableFilter->setMinimumHeight(32);  // 从36减小到32
    m_chkEnableFilter->setChecked(false);
    mainLayout->addWidget(m_chkEnableFilter);
    
    qDebug() << "✅ 创建复选框:" << m_chkEnableFilter 
             << ", 文本:" << m_chkEnableFilter->text()
             << ", 选中:" << m_chkEnableFilter->isChecked();
    
    // 创建滚动区域 - 包含其他所有过滤设置
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // 禁用横向滚动条
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);    // 禁用纵向滚动条
    scrollArea->setStyleSheet("QScrollArea { background-color: white; border: none; }");
    
    QWidget *scrollContent = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(scrollContent);
    layout->setSpacing(8);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // 过滤模式选择 - 使用QButtonGroup确保互斥
    QGroupBox *modeGroup = new QGroupBox("过滤模式");
    modeGroup->setStyleSheet(StyleManager::groupBoxStyle());
    QVBoxLayout *modeLayout = new QVBoxLayout(modeGroup);
    modeLayout->setSpacing(5);
    modeLayout->setContentsMargins(10, 12, 10, 8);
    
    m_rbFilterOnReceive = new QRadioButton("接收时过滤（节省内存，数据丢失）");
    m_rbFilterOnDisplay = new QRadioButton("显示时过滤（保留数据，可修改规则）");
    
    // 创建按钮组确保互斥
    QButtonGroup *modeButtonGroup = new QButtonGroup(scrollContent);
    modeButtonGroup->addButton(m_rbFilterOnReceive);
    modeButtonGroup->addButton(m_rbFilterOnDisplay);
    
    modeLayout->addWidget(m_rbFilterOnReceive);
    modeLayout->addWidget(m_rbFilterOnDisplay);
    
    layout->addWidget(modeGroup);
    
    // 帧ID和数据格式合并到一行 - 使用QButtonGroup确保互斥
    QHBoxLayout *idFormatLayout = new QHBoxLayout();
    QLabel *lblTitle = new QLabel("帧ID:");
    lblTitle->setStyleSheet("font-weight: bold; font-size: 10pt;");
    
    m_rbHex = new QRadioButton("数字(Hex)");
    m_rbString = new QRadioButton("字符串");
    
    // 创建按钮组确保互斥
    QButtonGroup *formatGroup = new QButtonGroup(scrollContent);
    formatGroup->addButton(m_rbHex);
    formatGroup->addButton(m_rbString);
    
    idFormatLayout->addWidget(lblTitle);
    idFormatLayout->addSpacing(10);
    idFormatLayout->addWidget(m_rbHex);
    idFormatLayout->addWidget(m_rbString);
    idFormatLayout->addStretch();
    
    connect(m_rbHex, &QRadioButton::toggled, this, &CANViewSettingsDialog::onFilterModeChanged);
    
    layout->addLayout(idFormatLayout);
    
    // 第一个条件
    QHBoxLayout *cond1Layout = new QHBoxLayout();
    
    m_comboOp1 = new CustomComboBox();
    m_comboOp1->addItem("等于");
    m_comboOp1->addItem("不等于");
    m_comboOp1->addItem("大于");
    m_comboOp1->addItem("小于");
    m_comboOp1->addItem("大于等于");
    m_comboOp1->addItem("小于等于");
    m_comboOp1->setMinimumWidth(100);
    
    m_editValue1 = new QLineEdit();
    m_editValue1->setPlaceholderText("输入帧ID（十六进制）");
    
    cond1Layout->addWidget(m_comboOp1);
    cond1Layout->addWidget(m_editValue1);
    
    layout->addLayout(cond1Layout);
    
    // 逻辑运算符 - 使用QButtonGroup确保互斥
    QHBoxLayout *logicLayout = new QHBoxLayout();
    
    m_rbAnd = new QRadioButton("与");
    m_rbOr = new QRadioButton("或");
    
    // 创建按钮组确保互斥
    QButtonGroup *logicGroup = new QButtonGroup(scrollContent);
    logicGroup->addButton(m_rbAnd);
    logicGroup->addButton(m_rbOr);
    
    logicLayout->addWidget(m_rbAnd);
    logicLayout->addWidget(m_rbOr);
    logicLayout->addStretch();
    
    layout->addLayout(logicLayout);
    
    // 第二个条件
    QHBoxLayout *cond2Layout = new QHBoxLayout();
    
    m_comboOp2 = new CustomComboBox();
    m_comboOp2->addItem("等于");
    m_comboOp2->addItem("不等于");
    m_comboOp2->addItem("大于");
    m_comboOp2->addItem("小于");
    m_comboOp2->addItem("大于等于");
    m_comboOp2->addItem("小于等于");
    m_comboOp2->setMinimumWidth(100);
    
    m_editValue2 = new QLineEdit();
    m_editValue2->setPlaceholderText("输入帧ID（十六进制，可选）");
    
    cond2Layout->addWidget(m_comboOp2);
    cond2Layout->addWidget(m_editValue2);
    
    layout->addLayout(cond2Layout);
    
    // 注意事项 - 优化样式
    m_lblFilterNote = new QLabel();
    m_lblFilterNote->setWordWrap(true);
    m_lblFilterNote->setStyleSheet(
        "QLabel { "
        "   background-color: #E3F2FD; "
        "   border: 1px solid #90CAF9; "
        "   padding: 10px; "
        "   border-radius: 4px; "
        "   font-size: 9pt; "
        "   line-height: 1.4; "  // 增加行高，让文字更易读
        "}"
        "code { "  // 为代码样式添加背景
        "   background-color: #BBDEFB; "
        "   padding: 2px 4px; "
        "   border-radius: 2px; "
        "   font-family: 'Consolas', 'Courier New', monospace; "
        "}"
    );
    updateFilterNoteText();
    
    layout->addWidget(m_lblFilterNote);
    
    // 默认选中"或"、"数字(Hex)"、"显示时过滤"
    m_rbOr->setChecked(true);
    m_rbHex->setChecked(true);
    m_rbFilterOnDisplay->setChecked(true);
    
    // 将内容添加到滚动区域
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);
    
    return widget;
}

void CANViewSettingsDialog::updateFilterNoteText()
{
    QString noteText;
    
    if (m_rbHex && m_rbHex->isChecked()) {
        noteText = "<b>注意：</b><br>"
                   "<b>1. 数字模式规则：</b><br>"
                   "   • 只能填写十六进制数字<br>"
                   "   • 等于/不等于支持多个值，用逗号分隔（如：123, 345）<br>"
                   "<b>2. 示例：</b><br>"
                   "   • <code>等于 123</code> → 只显示ID为0x123的帧<br>"
                   "   • <code>大于 100</code> → 显示ID大于0x100的帧<br>"
                   "   • <code>等于 123 或 等于 456</code> → 显示ID为0x123或0x456的帧";
    } else {
        noteText = "<b>注意：</b><br>"
                   "<b>1. 字符串模式规则：</b><br>"
                   "   • 支持通配符：<code>?</code> 代表单个字符，<code>*</code> 代表任意多个字符<br>"
                   "   • 不含通配符时为精确匹配<br>"
                   "<b>2. 示例：</b><br>"
                   "   • <code>111</code> → 只匹配111<br>"
                   "   • <code>*111*</code> → 匹配包含111的所有ID<br>"
                   "   • <code>1?3</code> → 匹配103, 113, 123等";
    }
    
    if (m_lblFilterNote) {
        m_lblFilterNote->setText(noteText);
    }
}

void CANViewSettingsDialog::onFilterModeChanged()
{
    updateFilterNoteText();
    
    // 更新占位符文本
    if (m_rbHex->isChecked()) {
        m_editValue1->setPlaceholderText("输入帧ID（十六进制）");
        m_editValue2->setPlaceholderText("输入帧ID（十六进制，可选）");
    } else {
        m_editValue1->setPlaceholderText("输入字符串（支持通配符 ? 和 *）");
        m_editValue2->setPlaceholderText("输入字符串（支持通配符 ? 和 *，可选）");
    }
}

IDFilterRule CANViewSettingsDialog::getFilterRule() const
{
    IDFilterRule rule;
    
    rule.enabled = m_chkEnableFilter->isChecked();
    rule.mode = m_rbFilterOnReceive->isChecked() ? 
                IDFilterRule::FilterOnReceive : IDFilterRule::FilterOnDisplay;
    rule.isHexMode = m_rbHex->isChecked();
    rule.value1 = m_editValue1->text().trimmed();
    rule.value2 = m_editValue2->text().trimmed();
    
    qDebug() << "📋 获取过滤规则:";
    qDebug() << "   启用:" << rule.enabled;
    qDebug() << "   模式:" << (rule.isHexMode ? "数字(Hex)" : "字符串");
    qDebug() << "   值1:" << rule.value1;
    qDebug() << "   值2:" << rule.value2;
    
    // 第一个运算符
    switch (m_comboOp1->currentIndex()) {
        case 0: rule.compareOp1 = IDFilterRule::Equal; break;
        case 1: rule.compareOp1 = IDFilterRule::NotEqual; break;
        case 2: rule.compareOp1 = IDFilterRule::GreaterThan; break;
        case 3: rule.compareOp1 = IDFilterRule::LessThan; break;
        case 4: rule.compareOp1 = IDFilterRule::GreaterEqual; break;
        case 5: rule.compareOp1 = IDFilterRule::LessEqual; break;
    }
    
    // 逻辑运算符
    rule.logicOp = m_rbAnd->isChecked() ? IDFilterRule::And : IDFilterRule::Or;
    
    // 第二个运算符
    switch (m_comboOp2->currentIndex()) {
        case 0: rule.compareOp2 = IDFilterRule::Equal; break;
        case 1: rule.compareOp2 = IDFilterRule::NotEqual; break;
        case 2: rule.compareOp2 = IDFilterRule::GreaterThan; break;
        case 3: rule.compareOp2 = IDFilterRule::LessThan; break;
        case 4: rule.compareOp2 = IDFilterRule::GreaterEqual; break;
        case 5: rule.compareOp2 = IDFilterRule::LessEqual; break;
    }
    
    return rule;
}

void CANViewSettingsDialog::setFilterRule(const IDFilterRule &rule)
{
    m_chkEnableFilter->setChecked(rule.enabled);
    m_rbFilterOnReceive->setChecked(rule.mode == IDFilterRule::FilterOnReceive);
    m_rbFilterOnDisplay->setChecked(rule.mode == IDFilterRule::FilterOnDisplay);
    m_rbHex->setChecked(rule.isHexMode);
    m_rbString->setChecked(!rule.isHexMode);
    m_editValue1->setText(rule.value1);
    m_editValue2->setText(rule.value2);
    
    // 第一个运算符
    m_comboOp1->setCurrentIndex(static_cast<int>(rule.compareOp1));
    
    // 逻辑运算符
    m_rbAnd->setChecked(rule.logicOp == IDFilterRule::And);
    m_rbOr->setChecked(rule.logicOp == IDFilterRule::Or);
    
    // 第二个运算符
    m_comboOp2->setCurrentIndex(static_cast<int>(rule.compareOp2));
    
    onFilterModeChanged();
}
