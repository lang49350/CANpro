#include "DebugLogWindow.h"
#include "../core/LogManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QScrollBar>
#include <QStandardPaths>

DebugLogWindow* DebugLogWindow::s_instance = nullptr;
QtMessageHandler DebugLogWindow::s_previousHandler = nullptr;
bool DebugLogWindow::s_handlerInstalled = false;

DebugLogWindow::DebugLogWindow(QWidget *parent)
    : QWidget(parent)
    , m_autoScroll(true)
{
    setupUi();
    connect(LogManager::instance(), &LogManager::logMessage, this, &DebugLogWindow::appendLog);
}

DebugLogWindow::~DebugLogWindow()
{
    if (s_instance == this) {
        LogManager::setEnabled(false);
        uninstallMessageHandler();
        s_instance = nullptr;
    }
}

DebugLogWindow* DebugLogWindow::instance()
{
    if (!s_instance) {
        s_instance = new DebugLogWindow();
    }
    return s_instance;
}

void DebugLogWindow::log(const QString &message)
{
    if (s_instance) {
        s_instance->appendLog(message);
    }
}

void DebugLogWindow::setupUi()
{
    setWindowTitle("日志管理");
    resize(900, 650);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(10);

    m_chkEnabled = new QCheckBox("启用日志");
    m_chkEnabled->setChecked(false);
    m_chkEnabled->setStyleSheet("QCheckBox { font-weight: bold; }");
    connect(m_chkEnabled, &QCheckBox::toggled, this, &DebugLogWindow::onEnabledChanged);
    controlLayout->addWidget(m_chkEnabled);

    QLabel *lblLevel = new QLabel("级别:");
    lblLevel->setStyleSheet("font-weight: bold;");
    m_comboLevel = new QComboBox();
    m_comboLevel->addItem("DEBUG（全部）", (int)LogLevel::DEBUG);
    m_comboLevel->addItem("INFO（一般）", (int)LogLevel::INFO);
    m_comboLevel->addItem("WARNING（警告）", (int)LogLevel::WARNING);
    m_comboLevel->addItem("ERROR（错误）", (int)LogLevel::ERROR);

    LogLevel currentLevel = LogManager::getMinLevel();
    for (int i = 0; i < m_comboLevel->count(); i++) {
        if (m_comboLevel->itemData(i).toInt() == (int)currentLevel) {
            m_comboLevel->setCurrentIndex(i);
            break;
        }
    }

    m_comboLevel->setMinimumWidth(150);
    m_comboLevel->setEnabled(false);
    connect(m_comboLevel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DebugLogWindow::onLevelChanged);
    controlLayout->addWidget(lblLevel);
    controlLayout->addWidget(m_comboLevel);

    controlLayout->addStretch();

    m_chkAutoScroll = new QCheckBox("自动滚动");
    m_chkAutoScroll->setChecked(true);
    connect(m_chkAutoScroll, &QCheckBox::checkStateChanged, this, &DebugLogWindow::onAutoScrollChanged);
    controlLayout->addWidget(m_chkAutoScroll);

    m_btnClear = new QPushButton("清空");
    m_btnClear->setMinimumWidth(80);
    connect(m_btnClear, &QPushButton::clicked, this, &DebugLogWindow::onClearClicked);
    controlLayout->addWidget(m_btnClear);

    m_btnSave = new QPushButton("保存");
    m_btnSave->setMinimumWidth(80);
    connect(m_btnSave, &QPushButton::clicked, this, &DebugLogWindow::onSaveClicked);
    controlLayout->addWidget(m_btnSave);

    mainLayout->addLayout(controlLayout);

    m_logView = new QPlainTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setStyleSheet(
        "QPlainTextEdit { "
        "   background-color: #1E1E1E; "
        "   color: #D4D4D4; "
        "   font-family: 'Consolas', 'Courier New', monospace; "
        "   font-size: 9pt; "
        "   border: 1px solid #3C3C3C; "
        "}"
    );
    m_logView->setMaximumBlockCount(10000);
    mainLayout->addWidget(m_logView);
}

void DebugLogWindow::appendLog(const QString &message)
{
    m_logView->appendPlainText(message);

    if (m_autoScroll) {
        m_logView->verticalScrollBar()->setValue(m_logView->verticalScrollBar()->maximum());
    }
}

void DebugLogWindow::onEnabledChanged(bool enabled)
{
    const bool effectiveEnabled = enabled && isVisible();
    LogManager::setEnabled(effectiveEnabled);
    m_comboLevel->setEnabled(enabled);

    const QString separator(60, '=');
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    const QString msg = enabled
        ? QString("[%1] 日志系统已启用，仅在窗口打开期间记录日志").arg(timestamp)
        : QString("[%1] 日志系统已禁用").arg(timestamp);

    m_logView->appendPlainText(separator);
    m_logView->appendPlainText(msg);
    m_logView->appendPlainText(separator);

    if (m_autoScroll) {
        m_logView->verticalScrollBar()->setValue(m_logView->verticalScrollBar()->maximum());
    }
}

void DebugLogWindow::onLevelChanged(int index)
{
    LogLevel level = (LogLevel)m_comboLevel->itemData(index).toInt();
    LogManager::setMinLevel(level);

    const QString separator(60, '=');
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    const QString msg = QString("[%1] 日志级别已切换到: %2")
        .arg(timestamp)
        .arg(m_comboLevel->currentText());

    m_logView->appendPlainText(separator);
    m_logView->appendPlainText(msg);
    m_logView->appendPlainText(separator);

    if (m_autoScroll) {
        m_logView->verticalScrollBar()->setValue(m_logView->verticalScrollBar()->maximum());
    }
}

void DebugLogWindow::onClearClicked()
{
    m_logView->clear();
    appendLog("日志已清空");
}

void DebugLogWindow::onSaveClicked()
{
    QString defaultFileName = QString("log_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    QString fileName = QFileDialog::getSaveFileName(
        nullptr,
        "保存日志",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + defaultFileName,
        "文本文件 (*.txt);;所有文件 (*.*)",
        nullptr,
        QFileDialog::Options()
    );

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法保存文件: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    out << m_logView->toPlainText();
    file.close();

    appendLog(QString("日志已保存到: %1").arg(fileName));
    QMessageBox::information(this, "成功", "日志已保存");
}

void DebugLogWindow::onAutoScrollChanged(int state)
{
    m_autoScroll = (state == Qt::Checked);
}

void DebugLogWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    installMessageHandler();
    LogManager::setEnabled(m_chkEnabled->isChecked());
}

void DebugLogWindow::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    LogManager::setEnabled(false);
    uninstallMessageHandler();
}

void DebugLogWindow::installMessageHandler()
{
    if (s_handlerInstalled) {
        return;
    }

    s_previousHandler = qInstallMessageHandler(&DebugLogWindow::qtMessageRouter);
    s_handlerInstalled = true;
}

void DebugLogWindow::uninstallMessageHandler()
{
    if (!s_handlerInstalled) {
        return;
    }

    qInstallMessageHandler(s_previousHandler);
    s_handlerInstalled = false;
}

void DebugLogWindow::qtMessageRouter(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    if (!LogManager::isEnabled()) {
        return;
    }

    LogLevel level;
    switch (type) {
        case QtDebugMsg:    level = LogLevel::DEBUG; break;
        case QtInfoMsg:     level = LogLevel::INFO; break;
        case QtWarningMsg:  level = LogLevel::WARNING; break;
        case QtCriticalMsg: level = LogLevel::ERROR; break;
        case QtFatalMsg:    level = LogLevel::ERROR; break;
        default:            level = LogLevel::INFO; break;
    }

    if (level < LogManager::getMinLevel()) {
        return;
    }

    LogManager::instance()->log(level, LogCategory::GENERAL, msg);
}
