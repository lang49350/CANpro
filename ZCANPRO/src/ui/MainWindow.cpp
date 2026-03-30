#include "MainWindow.h"
#include "CANViewWidget.h"
#include "FramePlaybackWidget.h"
#include "FrameSenderWidget.h"
#include "CustomMdiSubWindow.h"
#include "CustomComboBox.h"
#include "DeviceManagerDialog.h"
#include "DebugLogWindow.h"
#include "AnimatedToolButton.h"  // 添加动画按钮
#include "StatisticsDialog.h"
#include "ProtocolConfigDialog.h"
#include "../core/ConnectionManager.h"
#include "../core/DataRouter.h"
#include "../core/FrameStorageEngine.h"
#include "../core/LogManager.h"
#include "../core/zcan/ZCANStatistics.h"
#include "../core/zcan/zcan_types.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QWidget>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QHeaderView>
#include <QToolButton>
#include <QProcess>
#include <QCoreApplication>
#include <QTableWidget>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QMouseEvent>
#include <QHoverEvent>
#include <QResizeEvent>
#include <QRegularExpressionValidator>
#include <QGraphicsDropShadowEffect>  // 添加阴影效果

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_hasShownDisconnectError(false)
    , m_dragging(false)
    , m_resizing(false)
    , m_resizeEdge(None)
    , m_borderWidth(5)
    , m_canWindowCount(0)        // 🔧 初始化CAN窗口数量
    , m_dbcWindowCount(0)        // 🔧 初始化DBC窗口数量
    , m_j1939WindowCount(0)      // 🔧 初始化J1939窗口数量
    , m_waveformWindowCount(0)   // 🔧 初始化波形窗口数量
    , m_playbackWindowCount(0)   // 🔧 初始化回放窗口数量
    , m_canWindowNextId(1)       // 🔧 初始化CAN窗口编号（从1开始）
    , m_dbcWindowNextId(1)       // 🔧 初始化DBC窗口编号
    , m_j1939WindowNextId(1)     // 🔧 初始化J1939窗口编号
    , m_waveformWindowNextId(1)  // 🔧 初始化波形窗口编号
    , m_playbackWindowNextId(1)  // 🔧 初始化回放窗口编号
{
    // 设置无边框窗口
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, false);
    
    // 设置窗口图标
    setWindowIcon(QIcon(":/logo.png"));
    
    // 启用Hover事件，用于边框光标检测
    setAttribute(Qt::WA_Hover, true);
    
    // 🎨 加载应用程序样式表
    loadStyleSheet();
    
    setupUi();
    setupConnections();
    setupStatusBar();
    loadSettings();
    
    // 创建更新定时器（只更新状态栏）
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    m_updateTimer->start(100); // 每100ms更新一次状态栏
    
    // 创建窗口大小改变延迟重排定时器
    m_resizeTimer = new QTimer(this);
    m_resizeTimer->setSingleShot(true);  // 单次触发
    connect(m_resizeTimer, &QTimer::timeout, this, &MainWindow::arrangeSubWindows);
    
    // 默认打开一个CAN视图
    QTimer::singleShot(100, this, &MainWindow::onNewCANView);
    
    qDebug() << "✅ 主窗口初始化完成 - 新架构模式";
    qDebug() << "   使用ConnectionManager管理设备连接";
    qDebug() << "   使用DataRouter路由数据";
    qDebug() << "   使用CANViewWidget独立窗口";
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUi()
{
    // 设置窗口标题和图标
    setWindowTitle("CANAnalyzerPro - 专业CAN总线分析工具");
    setWindowIcon(QIcon(":/logo.png"));  // 设置窗口图标
    resize(1000, 680);  // 从1200x720减小到1000x680
    
    // 创建中央部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 1. 顶部红色工具栏（带下拉菜单的图标按钮）
    setupToolbar();
    
    // 2. MDI区域（多文档界面）
    m_mdiArea = new QMdiArea();
    m_mdiArea->setStyleSheet("QMdiArea { background-color: #E0E0E0; }");
    m_mdiArea->setViewMode(QMdiArea::SubWindowView);
    mainLayout->addWidget(m_mdiArea);
    
    // 3. 状态栏
    statusBar = new QStatusBar();
    statusBar->setStyleSheet("background-color: #37474F; color: white;");
    setStatusBar(statusBar);
}

void MainWindow::setupToolbar()
{
    QWidget *topToolbar = new QWidget();
    topToolbar->setMinimumHeight(90);  // 从80增加到90
    topToolbar->setMaximumHeight(90);
    topToolbar->setStyleSheet("QWidget { background-color: #E74C3C; }");  // 淡一些的红色
    
    QHBoxLayout *topLayout = new QHBoxLayout(topToolbar);
    topLayout->setSpacing(5);
    topLayout->setContentsMargins(10, 5, 10, 5);
    
    // LOGO区域 - 图标 + 文字
    QWidget *logoWidget = new QWidget();
    QHBoxLayout *logoLayout = new QHBoxLayout(logoWidget);
    logoLayout->setSpacing(8);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    
    // Logo图标
    QLabel *lblIcon = new QLabel();
    QPixmap logoPixmap(":/logo_transparent.png");
    // 缩放到更大的尺寸，几乎占满工具栏高度
    int iconSize = 72; // 从64增加到72像素
    lblIcon->setPixmap(logoPixmap.scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    lblIcon->setAlignment(Qt::AlignCenter);
    lblIcon->setAttribute(Qt::WA_TransparentForMouseEvents);  // 允许鼠标事件穿透，以便拖动窗口
    logoLayout->addWidget(lblIcon);
    
    // Logo文字
    QLabel *lblText = new QLabel("<font color='white' size='6'><b>ZCANPRO</b></font>");
    lblText->setAlignment(Qt::AlignVCenter);
    lblText->setAttribute(Qt::WA_TransparentForMouseEvents);  // 允许鼠标事件穿透，以便拖动窗口
    logoLayout->addWidget(lblText);
    
    topLayout->addWidget(logoWidget);
    topLayout->addSpacing(30);
    
    // 工具按钮样式
    QString toolBtnStyle = 
        "QToolButton { "
        "   background-color: rgba(0,0,0,0.15); "
        "   color: white; "
        "   border: 1px solid rgba(255,255,255,0.1); "  // 微妙边框
        "   padding-top: 10px; "
        "   padding-bottom: 6px; "
        "   padding-left: 12px; "  // 从15减少到12
        "   padding-right: 12px; "
        "   font-size: 13px; "
        "   font-weight: normal; "
        "   font-family: 'Microsoft YaHei UI', 'Microsoft YaHei', sans-serif; "
        "   border-radius: 4px; "
        "   min-width: 75px; "  // 从90减少到75
        "} "
        "QToolButton:hover { "
        "   background-color: rgba(255,255,255,0.25); "
        "   border: 1px solid rgba(255,255,255,0.5); "  // 更明显的边框
        "   padding-top: 10px; "
        "   padding-bottom: 6px; "
        "   padding-left: 12px; "
        "   padding-right: 12px; "
        "} "
        "QToolButton:pressed { "
        "   background-color: rgba(255,255,255,0.35); "
        "   border: 1px solid rgba(255,255,255,0.6); "
        "   padding-top: 10px; "
        "   padding-bottom: 6px; "
        "   padding-left: 12px; "
        "   padding-right: 12px; "
        "} "
        "QToolButton::menu-indicator { "
        "   image: none; "
        "   width: 0px; "
        "}";
    
    // 统一的菜单样式（白色背景）
    QString menuStyle = 
        "QMenu { "
        "   background-color: white; "
        "   color: black; "
        "   border: 1px solid #CCCCCC; "
        "   padding: 5px; "
        "} "
        "QMenu::item { "
        "   padding: 8px 30px; "
        "   background-color: transparent; "
        "} "
        "QMenu::item:selected { "
        "   background-color: #E3F2FD; "
        "   color: black; "
        "} "
        "QMenu::separator { "
        "   height: 1px; "
        "   background-color: #E0E0E0; "
        "   margin: 5px 0px; "
        "}";
    
    // 1. 设备管理
    AnimatedToolButton *btnDevice = new AnimatedToolButton();
    btnDevice->setText("设备管理");
    btnDevice->setIcon(QIcon(":/device.png"));
    btnDevice->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnDevice->setIconSize(QSize(40, 40));
    btnDevice->setStyleSheet(toolBtnStyle);
    btnDevice->setPopupMode(QToolButton::InstantPopup);
    
    QMenu *menuDevice = new QMenu(btnDevice);
    menuDevice->setStyleSheet(menuStyle);
    menuDevice->addAction("设备配置", this, &MainWindow::onConnectClicked);
    menuDevice->addAction("固件升级");
    btnDevice->setMenu(menuDevice);
    topLayout->addWidget(btnDevice);
    
    // 2. 新建视图
    AnimatedToolButton *btnView = new AnimatedToolButton();
    btnView->setText("新建视图");
    btnView->setIcon(QIcon(":/view.png"));
    btnView->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnView->setIconSize(QSize(40, 40));
    btnView->setStyleSheet(toolBtnStyle);
    btnView->setPopupMode(QToolButton::InstantPopup);
    QMenu *menuView = new QMenu(btnView);
    menuView->setStyleSheet(menuStyle);
    menuView->addAction("新建CAN视图", this, &MainWindow::onNewCANView);
    menuView->addAction("新建DBC视图", this, &MainWindow::onNewDBCView);
    menuView->addAction("新建CANOpen视图");
    menuView->addAction("新建SAE J1939视图", this, &MainWindow::onNewJ1939View);
    menuView->addAction("新建DeviceNet视图");
    menuView->addAction("新建TEXT视图");
    btnView->setMenu(menuView);
    topLayout->addWidget(btnView);
    
    // 3. 发送数据
    AnimatedToolButton *btnSend = new AnimatedToolButton();
    btnSend->setText("发送数据");
    btnSend->setIcon(QIcon(":/send.png"));
    btnSend->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnSend->setIconSize(QSize(40, 40));
    btnSend->setStyleSheet(toolBtnStyle);
    btnSend->setPopupMode(QToolButton::InstantPopup);
    QMenu *menuSend = new QMenu(btnSend);
    menuSend->setStyleSheet(menuStyle);
    menuSend->addAction("数据回放", this, &MainWindow::onDataPlayback);
    menuSend->addAction("周期发送", this, &MainWindow::onFrameSender);
    menuSend->addAction("发送列表");
    menuSend->addAction("发送脚本");
    btnSend->setMenu(menuSend);
    topLayout->addWidget(btnSend);
    
    // 4. 性能监控
    AnimatedToolButton *btnChannel = new AnimatedToolButton();
    btnChannel->setText("性能监控");
    btnChannel->setIcon(QIcon(":/channel.png"));
    btnChannel->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnChannel->setIconSize(QSize(40, 40));
    btnChannel->setStyleSheet(toolBtnStyle);
    btnChannel->setPopupMode(QToolButton::InstantPopup);
    QMenu *menuChannel = new QMenu(btnChannel);
    menuChannel->setStyleSheet(menuStyle);
    menuChannel->addAction("性能统计", this, &MainWindow::onShowStatistics);
    menuChannel->addAction("协议模式设置", this, &MainWindow::onShowProtocolConfig);
    btnChannel->setMenu(menuChannel);
    topLayout->addWidget(btnChannel);
    
    // 5. 高级功能
    AnimatedToolButton *btnAdvanced = new AnimatedToolButton();
    btnAdvanced->setText("高级功能");
    btnAdvanced->setIcon(QIcon(":/advanced.png"));
    btnAdvanced->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnAdvanced->setIconSize(QSize(40, 40));
    btnAdvanced->setStyleSheet(toolBtnStyle);
    btnAdvanced->setPopupMode(QToolButton::InstantPopup);
    QMenu *menuAdvanced = new QMenu(btnAdvanced);
    menuAdvanced->setStyleSheet(menuStyle);
    menuAdvanced->addAction("触发配置");
    menuAdvanced->addAction("数据统计");
    menuAdvanced->addAction("数据回放");
    menuAdvanced->addAction("实时曲线分析", this, &MainWindow::onNewWaveformView);
    menuAdvanced->addSeparator();
    menuAdvanced->addAction("自动化测试");
    menuAdvanced->addAction("GPS轨迹");
    menuAdvanced->addAction("车辆识别代号");
    menuAdvanced->addSeparator();
    menuAdvanced->addAction("UDS诊断");
    menuAdvanced->addAction("OBD诊断");
    menuAdvanced->addAction("ECU刷新");
    menuAdvanced->addAction("J1939 DTC监测");
    menuAdvanced->addSeparator();
    menuAdvanced->addAction("LIN数据收发");
    menuAdvanced->addAction("扩展脚本");
    btnAdvanced->setMenu(menuAdvanced);
    topLayout->addWidget(btnAdvanced);
    
    // 6. 工具
    AnimatedToolButton *btnTools = new AnimatedToolButton();
    btnTools->setText("工具");
    btnTools->setIcon(QIcon(":/tools.png"));
    btnTools->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnTools->setIconSize(QSize(40, 40));
    btnTools->setStyleSheet(toolBtnStyle);
    btnTools->setPopupMode(QToolButton::InstantPopup);
    QMenu *menuTools = new QMenu(btnTools);
    menuTools->setStyleSheet(
        "QMenu { "
        "   background-color: white; "
        "   color: black; "
        "   border: 1px solid #CCCCCC; "
        "   padding: 5px; "
        "} "
        "QMenu::item { "
        "   padding: 8px 30px; "
        "   background-color: transparent; "
        "} "
        "QMenu::item:selected { "
        "   background-color: #E3F2FD; "
        "   color: black; "
        "} "
        "QMenu::separator { "
        "   height: 1px; "
        "   background-color: #E0E0E0; "
        "   margin: 5px 0px; "
        "}"
    );
    menuTools->addAction("TCPUDP调试工具", this, [this]() {
        QString toolDir = QCoreApplication::applicationDirPath() + "/tools/TCP&UDPDebug";
        QString toolPath = toolDir + "/TCPUDPDbg.exe";
        qDebug() << "启动工具:" << toolPath;
        qDebug() << "工作目录:" << toolDir;
        
        // 使用QProcess并设置工作目录
        QProcess *process = new QProcess(this);
        process->setProgram(toolPath);
        process->setWorkingDirectory(toolDir);
        
        if (!process->startDetached()) {
            QMessageBox::warning(this, "错误", "无法启动TCPUDP调试工具\n路径: " + toolPath + "\n\n请检查文件是否存在");
        } else {
            delete process;
        }
    });
    menuTools->addAction("网络设备配置工具", this, [this]() {
        QString toolPath = QCoreApplication::applicationDirPath() + "/tools/CANetConfigurer.exe";
        QString workDir = QCoreApplication::applicationDirPath() + "/tools";
        qDebug() << "启动工具:" << toolPath;
        
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workDir);
        if (!process->startDetached(toolPath, QStringList(), workDir)) {
            QMessageBox::warning(this, "错误", "无法启动网络设备配置工具\n路径: " + toolPath);
        }
    });
    menuTools->addAction("CANFDCOM配置工具", this, [this]() {
        QString toolPath = QCoreApplication::applicationDirPath() + "/tools/CANFDCOM.exe";
        QString workDir = QCoreApplication::applicationDirPath() + "/tools";
        qDebug() << "启动工具:" << toolPath;
        
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workDir);
        if (!process->startDetached(toolPath, QStringList(), workDir)) {
            QMessageBox::warning(this, "错误", "无法启动CANFDCOM配置工具\n路径: " + toolPath);
        }
    });
    menuTools->addSeparator();
    menuTools->addAction("校验码计算工具", this, [this]() {
        QString toolPath = QCoreApplication::applicationDirPath() + "/tools/CheckCodeTool.exe";
        QString workDir = QCoreApplication::applicationDirPath() + "/tools";
        qDebug() << "启动工具:" << toolPath;
        
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workDir);
        if (!process->startDetached(toolPath, QStringList(), workDir)) {
            QMessageBox::warning(this, "错误", "无法启动校验码计算工具\n路径: " + toolPath);
        }
    });
    menuTools->addAction("SA计算工具", this, [this]() {
        QString toolPath = QCoreApplication::applicationDirPath() + "/tools/SATool.exe";
        QString workDir = QCoreApplication::applicationDirPath() + "/tools";
        qDebug() << "启动工具:" << toolPath;
        
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workDir);
        if (!process->startDetached(toolPath, QStringList(), workDir)) {
            QMessageBox::warning(this, "错误", "无法启动SA计算工具\n路径: " + toolPath);
        }
    });
    menuTools->addAction("波特率计算器", this, [this]() {
        QString toolPath = QCoreApplication::applicationDirPath() + "/tools/ZBaudcalTool.exe";
        QString workDir = QCoreApplication::applicationDirPath() + "/tools";
        qDebug() << "启动工具:" << toolPath;
        
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workDir);
        if (!process->startDetached(toolPath, QStringList(), workDir)) {
            QMessageBox::warning(this, "错误", "无法启动波特率计算器\n路径: " + toolPath);
        }
    });
    menuTools->addAction("文本转换工具", this, [this]() {
        QString toolPath = QCoreApplication::applicationDirPath() + "/tools/CharHexTool.exe";
        QString workDir = QCoreApplication::applicationDirPath() + "/tools";
        qDebug() << "启动工具:" << toolPath;
        
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workDir);
        if (!process->startDetached(toolPath, QStringList(), workDir)) {
            QMessageBox::warning(this, "错误", "无法启动文本转换工具\n路径: " + toolPath);
        }
    });
    menuTools->addSeparator();
    menuTools->addAction("格式转换工具", this, [this]() {
        // 调用外部的格式转换器exe文件
        QString toolPath = QCoreApplication::applicationDirPath() + "/tools/format_converter.exe";
        QString workDir = QCoreApplication::applicationDirPath() + "/tools";
        qDebug() << "启动格式转换工具:" << toolPath;
        
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workDir);
        if (!process->startDetached(toolPath, QStringList(), workDir)) {
            QMessageBox::warning(this, "错误", 
                "无法启动格式转换工具\n\n"
                "请确保format_converter.exe文件存在\n\n"
                "路径: " + toolPath);
            qDebug() << "❌ 启动失败";
        } else {
            qDebug() << "✅ 格式转换工具启动成功";
        }
    });
    btnTools->setMenu(menuTools);
    topLayout->addWidget(btnTools);
    
    // 7. 设置&帮助
    AnimatedToolButton *btnSettings = new AnimatedToolButton();
    btnSettings->setText("设置&帮助");
    btnSettings->setIcon(QIcon(":/settings.png"));
    btnSettings->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnSettings->setIconSize(QSize(40, 40));
    btnSettings->setStyleSheet(toolBtnStyle);
    btnSettings->setPopupMode(QToolButton::InstantPopup);
    QMenu *menuSettings = new QMenu(btnSettings);
    menuSettings->setStyleSheet(menuStyle);
    menuSettings->addAction("系统设置");
    menuSettings->addAction("通道配置");
    menuSettings->addAction("界面主题");
    menuSettings->addSeparator();
    
    // 日志管理（统一的日志控制窗口）
    menuSettings->addAction("日志管理", this, [this]() {
        DebugLogWindow::instance()->show();
        DebugLogWindow::instance()->raise();
    });
    
    menuSettings->addSeparator();
    menuSettings->addAction("使用手册");
    menuSettings->addAction("关于软件");
    btnSettings->setMenu(menuSettings);
    topLayout->addWidget(btnSettings);
    
    topLayout->addStretch();
    
    // 窗口控制按钮
    QPushButton *btnMin = new QPushButton("—");
    m_btnMaximize = new QPushButton("□");  // 保存为成员变量
    QPushButton *btnClose = new QPushButton("✕");
    
    QString winBtnStyle = 
        "QPushButton { "
        "   background-color: transparent; "
        "   color: white; "
        "   border: none; "
        "   padding: 0px 15px; "  // 减少垂直padding从5px到0px
        "   font-size: 16px; "
        "   min-height: 30px; "    // 设置最小高度
        "   max-height: 30px; "    // 设置最大高度
        "} "
        "QPushButton:hover { "
        "   background-color: rgba(255,255,255,0.2); "
        "}";
    
    btnMin->setStyleSheet(winBtnStyle);
    m_btnMaximize->setStyleSheet(winBtnStyle);
    btnClose->setStyleSheet(winBtnStyle + "QPushButton:hover { background-color: #E81123; }");
    
    connect(btnMin, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(m_btnMaximize, &QPushButton::clicked, [this]() {
        if (isMaximized()) showNormal(); else showMaximized();
    });
    connect(btnClose, &QPushButton::clicked, this, &QWidget::close);
    
    topLayout->addWidget(btnMin);
    topLayout->addWidget(m_btnMaximize);
    topLayout->addWidget(btnClose);
    
    // 添加到主布局
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(centralWidget()->layout());
    if (mainLayout) {
        mainLayout->insertWidget(0, topToolbar);
    }
}

void MainWindow::setupConnections()
{
    // 🚀 连接ConnectionManager的信号
    ConnectionManager *connMgr = ConnectionManager::instance();
    
    connect(connMgr, &ConnectionManager::deviceConnected,
            this, [this](const QString &port) {
        qDebug() << "✅ 设备连接成功:" << port;
        m_statusLabel->setText("已连接: " + port);
        m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
        m_hasShownDisconnectError = false;  // 重置错误标志
        
        // 通知所有子窗口更新设备列表
        updateAllWindowsDeviceList();
    });
    
    connect(connMgr, &ConnectionManager::deviceDisconnected,
            this, [this](const QString &port) {
        qDebug() << "🔌 设备断开:" << port;
        m_statusLabel->setText("设备断开: " + port);
        m_statusLabel->setStyleSheet("color: orange; font-weight: bold;");
    });
    
    connect(connMgr, &ConnectionManager::errorOccurred,
            this, [this](const QString &port, const QString &error) {
        qWarning() << "❌ 设备" << port << "错误:" << error;
        m_statusLabel->setText("错误: " + error);
        m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
        
        // 只在连接断开时弹一次对话框
        if (error.contains("断开") || error.contains("断线") || 
            error.contains("ResourceError") || error.contains("DeviceNotFoundError")) {
            if (!m_hasShownDisconnectError) {
                QMessageBox::warning(this, "连接断开", "系统连接已经断开！");
                m_hasShownDisconnectError = true;
            }
        }
    });
    
    // 🚀 连接ConnectionManager到DataRouter
    DataRouter *router = DataRouter::instance();
    connect(connMgr, &ConnectionManager::framesReceived,
            router, &DataRouter::routeFrames);

    FrameStorageEngine *storage = FrameStorageEngine::instance();
    connect(connMgr, &ConnectionManager::framesReceived,
            this, [storage](const QString &port, const QVector<CANFrame> &frames) {
        storage->enqueueFrames(port, frames);
    });
}

void MainWindow::onNewCANView()
{
    // 🔧 BUG修复：使用窗口数量限制最多10个
    if (m_canWindowCount >= 10) {
        QMessageBox::warning(this, QString::fromUtf8("提示"), 
            QString::fromUtf8("最多只能创建10个视图窗口\n"
                             "当前已有10个窗口，请关闭一些窗口后再试"));
        qDebug() << "⚠️ 已达到最大窗口数量限制（10个）";
        return;
    }
    
    // 🚀 创建CANViewWidget（独立窗口）
    CANViewWidget *viewWidget = new CANViewWidget();
    
    // 使用自定义MDI子窗口
    CustomMdiSubWindow *subWindow = new CustomMdiSubWindow();
    subWindow->setWidget(viewWidget);
    
    // 🔧 使用编号计数器设置窗口标题（确保编号唯一）
    int windowId = m_canWindowNextId;
    subWindow->setWindowTitle(QString("视图%1: CAN").arg(windowId));
    
    // 🔧 增加窗口数量和编号
    m_canWindowCount++;      // 当前窗口数量+1
    m_canWindowNextId++;     // 下一个窗口编号+1（永远递增）
    
    // 添加到MDI区域
    m_mdiArea->addSubWindow(subWindow);
    subWindow->resize(900, 650);
    subWindow->show();
    
    // 🔧 监听窗口销毁事件，只减少窗口数量（不减少编号）
    connect(subWindow, &QObject::destroyed, this, [this]() {
        m_canWindowCount--;  // 只减少数量
        qDebug() << "🗑 CAN窗口已销毁，当前CAN窗口数:" << m_canWindowCount;
        QTimer::singleShot(100, this, &MainWindow::arrangeSubWindows);
    });
    
    // 延迟自动排列窗口（避免频繁重排）
    m_resizeTimer->start(100);
    
    qDebug() << "✅ 创建新CAN视图，编号:" << windowId << "，当前窗口数:" << m_canWindowCount;
}

void MainWindow::onNewDBCView()
{
    // 🔧 BUG修复：使用独立计数器
    if (m_dbcWindowCount >= 10) {
        QMessageBox::warning(this, QString::fromUtf8("提示"), 
            QString::fromUtf8("最多只能创建10个视图窗口"));
        qDebug() << "⚠️ 已达到最大窗口数量限制（10个）";
        return;
    }
    
    // 统一的下拉框样式
    QString comboBoxStyle = 
        "QComboBox { "
        "   border: 1px solid #CCCCCC; "
        "   padding: 3px 20px 3px 5px; "
        "   background-color: white; "
        "} "
        "QComboBox::drop-down { "
        "   subcontrol-origin: padding; "
        "   subcontrol-position: top right; "
        "   width: 20px; "
        "   border-left: 1px solid #CCCCCC; "
        "}";
    
    // 创建DBC视图部件
    QWidget *viewWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(viewWidget);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // 1. 工具栏（浅灰色背景）
    QWidget *toolbar = new QWidget();
    toolbar->setMinimumHeight(45);
    toolbar->setMaximumHeight(45);
    toolbar->setStyleSheet("QWidget { background-color: #E8EAF6; }");
    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(10, 5, 10, 5);
    
    // 设备选择
    QLabel *lblDevice = new QLabel("请选择设备");
    CustomComboBox *comboDevice = new CustomComboBox();
    comboDevice->addItem("请选择设备");
    comboDevice->setMinimumWidth(150);
    
    // 物理/虚拟切换
    CustomComboBox *comboMode = new CustomComboBox();
    comboMode->addItem("物理");
    comboMode->addItem("虚拟");
    comboMode->setMinimumWidth(80);
    
    // 工具按钮
    QPushButton *btnLoadDBC = new QPushButton("📁 加载DBC");
    QPushButton *btnProperty = new QPushButton("📋 属性");
    QPushButton *btnPause = new QPushButton("⏸ 暂停");
    QPushButton *btnSave = new QPushButton("💾 保存");
    QPushButton *btnSettings = new QPushButton("⚙ 设置");
    
    toolbarLayout->addWidget(lblDevice);
    toolbarLayout->addWidget(comboDevice);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(comboMode);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(btnLoadDBC);
    toolbarLayout->addWidget(btnProperty);
    toolbarLayout->addWidget(btnPause);
    toolbarLayout->addWidget(btnSave);
    toolbarLayout->addWidget(btnSettings);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);
    
    // 2. 过滤器面板（白色背景）
    QWidget *filterPanel = new QWidget();
    filterPanel->setMinimumHeight(70);
    filterPanel->setMaximumHeight(70);
    filterPanel->setStyleSheet("QWidget { background-color: white; }");
    QGridLayout *filterLayout = new QGridLayout(filterPanel);
    filterLayout->setContentsMargins(10, 5, 10, 5);
    filterLayout->setSpacing(5);
    
    // 第一行标签
    QLabel *lblSeq = new QLabel("序号");
    QLabel *lblTime = new QLabel("时间");
    QLabel *lblMsgName = new QLabel("消息名");
    QLabel *lblVirtual = new QLabel("虚拟");
    QLabel *lblPGN = new QLabel("PGN(Hex)");
    QLabel *lblAddr = new QLabel("源/目标地址(Hex)");
    QLabel *lblFrameType = new QLabel("帧类型");
    QLabel *lblCount = new QLabel("计数");
    QLabel *lblDir = new QLabel("方向");
    QLabel *lblLen = new QLabel("长度");
    QLabel *lblData = new QLabel("数据(Hex)");
    
    filterLayout->addWidget(lblSeq, 0, 0);
    filterLayout->addWidget(lblTime, 0, 1);
    filterLayout->addWidget(lblMsgName, 0, 2);
    filterLayout->addWidget(lblVirtual, 0, 3);
    filterLayout->addWidget(lblPGN, 0, 4);
    filterLayout->addWidget(lblAddr, 0, 5);
    filterLayout->addWidget(lblFrameType, 0, 6);
    filterLayout->addWidget(lblCount, 0, 7);
    filterLayout->addWidget(lblDir, 0, 8);
    filterLayout->addWidget(lblLen, 0, 9);
    filterLayout->addWidget(lblData, 0, 10);
    
    // 第二行输入框
    QLineEdit *editSeq = new QLineEdit();
    QLineEdit *editTime = new QLineEdit();
    QLineEdit *editMsgName = new QLineEdit();
    QLineEdit *editVirtual = new QLineEdit();
    QLineEdit *editPGN = new QLineEdit();
    QLineEdit *editAddr = new QLineEdit();
    QLineEdit *editFrameType = new QLineEdit();
    QLineEdit *editCount = new QLineEdit();
    QLineEdit *editDir = new QLineEdit();
    QLineEdit *editLen = new QLineEdit();
    QLineEdit *editData = new QLineEdit();
    
    filterLayout->addWidget(editSeq, 1, 0);
    filterLayout->addWidget(editTime, 1, 1);
    filterLayout->addWidget(editMsgName, 1, 2);
    filterLayout->addWidget(editVirtual, 1, 3);
    filterLayout->addWidget(editPGN, 1, 4);
    filterLayout->addWidget(editAddr, 1, 5);
    filterLayout->addWidget(editFrameType, 1, 6);
    filterLayout->addWidget(editCount, 1, 7);
    filterLayout->addWidget(editDir, 1, 8);
    filterLayout->addWidget(editLen, 1, 9);
    filterLayout->addWidget(editData, 1, 10);
    
    // 设置列宽比例
    filterLayout->setColumnStretch(0, 1);   // 序号
    filterLayout->setColumnStretch(1, 2);   // 时间
    filterLayout->setColumnStretch(2, 2);   // 消息名
    filterLayout->setColumnStretch(3, 1);   // 虚拟
    filterLayout->setColumnStretch(4, 1);   // PGN
    filterLayout->setColumnStretch(5, 2);   // 源/目标地址
    filterLayout->setColumnStretch(6, 1);   // 帧类型
    filterLayout->setColumnStretch(7, 1);   // 计数
    filterLayout->setColumnStretch(8, 1);   // 方向
    filterLayout->setColumnStretch(9, 1);   // 长度
    filterLayout->setColumnStretch(10, 3);  // 数据
    
    layout->addWidget(filterPanel);
    
    // 3. 数据表格
    QTableWidget *tableWidget = new QTableWidget(0, 11);
    tableWidget->setHorizontalHeaderLabels(QStringList() 
        << "序号" << "时间" << "消息名" << "虚拟" << "PGN(Hex)" 
        << "源/目标地址(Hex)" << "帧类型" << "计数" << "方向" << "长度" << "数据(Hex)");
    
    tableWidget->setStyleSheet(
        "QTableWidget { "
        "   background-color: white; "
        "   gridline-color: #E0E0E0; "
        "   selection-background-color: #BBDEFB; "
        "} "
        "QHeaderView::section { "
        "   background-color: #CFD8DC; "
        "   padding: 5px; "
        "   border: 1px solid #B0BEC5; "
        "   font-weight: bold; "
        "}"
    );
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->horizontalHeader()->setStretchLastSection(true);
    
    // 设置列宽
    tableWidget->setColumnWidth(0, 60);   // 序号
    tableWidget->setColumnWidth(1, 120);  // 时间
    tableWidget->setColumnWidth(2, 150);  // 消息名
    tableWidget->setColumnWidth(3, 60);   // 虚拟
    tableWidget->setColumnWidth(4, 80);   // PGN
    tableWidget->setColumnWidth(5, 150);  // 源/目标地址
    tableWidget->setColumnWidth(6, 80);   // 帧类型
    tableWidget->setColumnWidth(7, 60);   // 计数
    tableWidget->setColumnWidth(8, 60);   // 方向
    tableWidget->setColumnWidth(9, 60);   // 长度
    
    layout->addWidget(tableWidget);
    
    // 4. 底部状态栏（浅灰色）
    QWidget *statusBar = new QWidget();
    statusBar->setMinimumHeight(30);
    statusBar->setMaximumHeight(30);
    statusBar->setStyleSheet("QWidget { background-color: #ECEFF1; }");
    QHBoxLayout *statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(10, 0, 10, 0);
    
    QLabel *lblStatus = new QLabel("未加载DBC文件");
    QLabel *lblMsgCount = new QLabel("消息数: 0");
    
    statusLayout->addWidget(lblStatus);
    statusLayout->addStretch();
    statusLayout->addWidget(lblMsgCount);
    
    layout->addWidget(statusBar);
    
    // 创建MDI子窗口
    CustomMdiSubWindow *subWindow = new CustomMdiSubWindow();
    subWindow->setWidget(viewWidget);
    
    // 🔧 使用编号计数器设置窗口标题（确保编号唯一）
    int windowId = m_dbcWindowNextId;
    subWindow->setWindowTitle(QString("视图%1: DBC").arg(windowId));
    
    // 🔧 增加窗口数量和编号
    m_dbcWindowCount++;      // 当前窗口数量+1
    m_dbcWindowNextId++;     // 下一个窗口编号+1（永远递增）
    
    // 添加到MDI区域
    m_mdiArea->addSubWindow(subWindow);
    subWindow->resize(1000, 600);
    subWindow->show();
    
    // 🔧 监听窗口销毁事件，只减少窗口数量（不减少编号）
    connect(subWindow, &QObject::destroyed, this, [this]() {
        m_dbcWindowCount--;  // 只减少数量
        qDebug() << "🗑 DBC窗口已销毁，当前DBC窗口数:" << m_dbcWindowCount;
        QTimer::singleShot(100, this, &MainWindow::arrangeSubWindows);
    });
    
    // 自动排列窗口
    arrangeSubWindows();
    
    qDebug() << "✅ 创建新DBC视图，编号:" << windowId << "，当前窗口数:" << m_dbcWindowCount;
}

void MainWindow::onNewJ1939View()
{
    // 🔧 BUG修复：使用窗口数量限制最多10个
    if (m_j1939WindowCount >= 10) {
        QMessageBox::warning(this, QString::fromUtf8("提示"), 
            QString::fromUtf8("最多只能创建10个视图窗口"));
        qDebug() << "⚠️ 已达到最大窗口数量限制（10个）";
        return;
    }
    
    QWidget *viewWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(viewWidget);
    QLabel *label = new QLabel("SAE J1939视图 - 待实现");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    
    CustomMdiSubWindow *subWindow = new CustomMdiSubWindow();
    subWindow->setWidget(viewWidget);
    
    // 🔧 使用编号计数器设置窗口标题（确保编号唯一）
    int windowId = m_j1939WindowNextId;
    subWindow->setWindowTitle(QString("视图%1: J1939").arg(windowId));
    
    // 🔧 增加窗口数量和编号
    m_j1939WindowCount++;      // 当前窗口数量+1
    m_j1939WindowNextId++;     // 下一个窗口编号+1（永远递增）
    
    m_mdiArea->addSubWindow(subWindow);
    subWindow->resize(600, 400);
    subWindow->show();
    
    // 🔧 监听窗口销毁事件，只减少窗口数量（不减少编号）
    connect(subWindow, &QObject::destroyed, this, [this]() {
        m_j1939WindowCount--;  // 只减少数量
        qDebug() << "🗑 J1939窗口已销毁，当前J1939窗口数:" << m_j1939WindowCount;
        QTimer::singleShot(100, this, &MainWindow::arrangeSubWindows);
    });
    
    arrangeSubWindows();
    qDebug() << "✅ 创建新J1939视图，编号:" << windowId << "，当前窗口数:" << m_j1939WindowCount;
}

void MainWindow::onNewWaveformView()
{
    // 🔧 BUG修复：使用窗口数量限制最多10个
    if (m_waveformWindowCount >= 10) {
        QMessageBox::warning(this, QString::fromUtf8("提示"), 
            QString::fromUtf8("最多只能创建10个视图窗口"));
        qDebug() << "⚠️ 已达到最大窗口数量限制（10个）";
        return;
    }
    
    QWidget *viewWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(viewWidget);
    QLabel *label = new QLabel("实时曲线分析 - 待实现");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    
    CustomMdiSubWindow *subWindow = new CustomMdiSubWindow();
    subWindow->setWidget(viewWidget);
    
    // 🔧 使用编号计数器设置窗口标题（确保编号唯一）
    int windowId = m_waveformWindowNextId;
    subWindow->setWindowTitle(QString("视图%1: 波形").arg(windowId));
    
    // 🔧 增加窗口数量和编号
    m_waveformWindowCount++;      // 当前窗口数量+1
    m_waveformWindowNextId++;     // 下一个窗口编号+1（永远递增）
    
    m_mdiArea->addSubWindow(subWindow);
    subWindow->resize(600, 400);
    subWindow->show();
    
    // 🔧 监听窗口销毁事件，只减少窗口数量（不减少编号）
    connect(subWindow, &QObject::destroyed, this, [this]() {
        m_waveformWindowCount--;  // 只减少数量
        qDebug() << "🗑 波形窗口已销毁，当前波形窗口数:" << m_waveformWindowCount;
        QTimer::singleShot(100, this, &MainWindow::arrangeSubWindows);
    });
    
    arrangeSubWindows();
    qDebug() << "✅ 创建新波形视图，编号:" << windowId << "，当前窗口数:" << m_waveformWindowCount;
}

void MainWindow::onDataPlayback()
{
    qDebug() << "📡 创建数据回放窗口";
    
    // 检查窗口数量限制（最多5个回放窗口）
    if (m_playbackWindowCount >= 5) {
        QMessageBox::warning(this, QString::fromUtf8("提示"), 
            QString::fromUtf8("最多只能创建5个回放窗口\n"
                             "当前已有5个窗口，请关闭一些窗口后再试"));
        qDebug() << "⚠️ 已达到最大回放窗口数量限制（5个）";
        return;
    }
    
    // 创建FramePlaybackWidget
    FramePlaybackWidget *playbackWidget = new FramePlaybackWidget();
    
    // 使用自定义MDI子窗口
    CustomMdiSubWindow *subWindow = new CustomMdiSubWindow();
    subWindow->setWidget(playbackWidget);
    
    // 使用编号计数器设置窗口标题（确保编号唯一）
    int windowId = m_playbackWindowNextId;
    subWindow->setWindowTitle(QString("回放%1").arg(windowId));
    
    // 增加窗口数量和编号
    m_playbackWindowCount++;      // 当前窗口数量+1
    m_playbackWindowNextId++;     // 下一个窗口编号+1（永远递增）
    
    // 添加到MDI区域
    m_mdiArea->addSubWindow(subWindow);
    subWindow->resize(900, 700);
    subWindow->show();
    
    // 监听窗口销毁事件，只减少窗口数量（不减少编号）
    connect(subWindow, &QObject::destroyed, this, [this]() {
        m_playbackWindowCount--;  // 只减少数量
        qDebug() << "🗑 回放窗口已销毁，当前回放窗口数:" << m_playbackWindowCount;
        QTimer::singleShot(100, this, &MainWindow::arrangeSubWindows);
    });
    
    // 延迟自动排列窗口（避免频繁重排）
    QTimer::singleShot(100, this, &MainWindow::arrangeSubWindows);
    
    qDebug() << "✅ 回放窗口创建成功，当前回放窗口数:" << m_playbackWindowCount;
}

/**
 * @brief 创建周期发送窗口
 */
void MainWindow::onFrameSender()
{
    qDebug() << "📡 创建周期发送窗口";
    
    // 创建FrameSenderWidget
    FrameSenderWidget *senderWidget = new FrameSenderWidget();
    
    // 使用自定义MDI子窗口
    CustomMdiSubWindow *subWindow = new CustomMdiSubWindow();
    subWindow->setWidget(senderWidget);
    subWindow->setWindowTitle("周期发送");
    
    // 添加到MDI区域
    m_mdiArea->addSubWindow(subWindow);
    subWindow->resize(900, 700);
    subWindow->show();
    
    // 监听窗口销毁事件
    connect(subWindow, &QObject::destroyed, this, [this]() {
        qDebug() << "🗑 发送窗口已销毁";
        QTimer::singleShot(100, this, &MainWindow::arrangeSubWindows);
    });
    
    // 延迟自动排列窗口（避免频繁重排）
    QTimer::singleShot(100, this, &MainWindow::arrangeSubWindows);
    
    qDebug() << "✅ 发送窗口创建成功";
}

void MainWindow::onShowStatistics()
{
    // 获取所有连接的设备
    ConnectionManager *connMgr = ConnectionManager::instance();
    QStringList devices = connMgr->connectedDevices();
    
    if (devices.isEmpty()) {
        QMessageBox::warning(this, "提示", "没有连接的设备");
        return;
    }
    
    // 构建设备连接映射
    QMap<QString, SerialConnection*> connections;
    for (const QString &device : devices) {
        SerialConnection *conn = connMgr->getConnection(device);
        if (conn) {
            connections[device] = conn;
        }
    }
    
    // 打开多标签页统计对话框
    StatisticsDialog dialog(connections, this);
    dialog.exec();
}

void MainWindow::onShowProtocolConfig()
{
    // 获取所有连接的设备
    ConnectionManager *connMgr = ConnectionManager::instance();
    QStringList devices = connMgr->connectedDevices();
    
    if (devices.isEmpty()) {
        QMessageBox::warning(this, "提示", "没有连接的设备");
        return;
    }
    
    // 构建设备连接映射
    QMap<QString, SerialConnection*> connections;
    for (const QString &device : devices) {
        SerialConnection *conn = connMgr->getConnection(device);
        if (conn) {
            connections[device] = conn;
        }
    }
    
    // 打开多标签页配置对话框
    ProtocolConfigDialog dialog(connections, this);
    dialog.exec();
}

void MainWindow::setupStatusBar()
{
    // 创建状态栏标签
    m_statusLabel = new QLabel("未连接");
    m_connectedDevicesLabel = new QLabel("设备: 无");
    m_protocolModeLabel = new QLabel("协议: --");
    m_throughputLabel = new QLabel("0 帧/秒");
    m_latencyLabel = new QLabel("延迟: --");
    m_memoryLabel = new QLabel("内存: OK");
    
    // 设置标签样式
    m_statusLabel->setStyleSheet("color: white; padding: 0 10px;");
    m_connectedDevicesLabel->setStyleSheet("color: white; padding: 0 10px;");
    m_protocolModeLabel->setStyleSheet("color: white; padding: 0 10px;");
    m_throughputLabel->setStyleSheet("color: white; padding: 0 10px;");
    m_latencyLabel->setStyleSheet("color: white; padding: 0 10px;");
    m_memoryLabel->setStyleSheet("color: white; padding: 0 10px;");
    
    // 添加到状态栏
    statusBar->addWidget(m_statusLabel);
    statusBar->addWidget(m_connectedDevicesLabel);
    statusBar->addPermanentWidget(m_protocolModeLabel);
    statusBar->addPermanentWidget(m_throughputLabel);
    statusBar->addPermanentWidget(m_latencyLabel);
    statusBar->addPermanentWidget(m_memoryLabel);
}

void MainWindow::onConnectClicked()
{
    // 🚀 打开设备管理对话框
    DeviceManagerDialog dialog(this);
    dialog.exec();
}

void MainWindow::onDisconnectClicked()
{
    // 🚀 使用ConnectionManager断开所有设备
    ConnectionManager *connMgr = ConnectionManager::instance();
    connMgr->disconnectAll();
    qDebug() << "🔌 断开所有设备";
}

void MainWindow::updateStatusBar()
{
    // 🚀 显示已连接的设备
    ConnectionManager *connMgr = ConnectionManager::instance();
    QStringList devices = connMgr->connectedDevices();
    
    if (devices.isEmpty()) {
        m_connectedDevicesLabel->setText("设备: 无");
        m_protocolModeLabel->setText("协议: --");
        m_throughputLabel->setText("0 帧/秒");
        m_latencyLabel->setText("延迟: --");
        m_lastRxDevice.clear();
        m_lastRxFrames = 0;
        m_lastRxSampleMs = 0;
        m_smoothedRxFps = -1.0;
    } else {
        m_connectedDevicesLabel->setText(QString("设备: %1").arg(devices.join(", ")));
        
        // 获取第一个设备的ZCAN统计
        SerialConnection *conn = connMgr->getConnection(devices.first());
        if (conn) {
            ZCAN::ZCANStatistics *stats = conn->getUCANStatistics();
            
            if (stats) {
                // 更新协议模式
                m_protocolModeLabel->setText(QStringLiteral("协议: UCAN-FD"));
                
                // 更新吞吐量（实时值：相邻采样增量）
                const auto metrics = stats->getMetrics();
                const quint32 currentFrames = metrics.framesReceived;
                const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
                double realtimeFps = 0.0;

                if (m_lastRxDevice == devices.first() && m_lastRxSampleMs > 0 && nowMs > m_lastRxSampleMs) {
                    const quint32 deltaFrames = (currentFrames >= m_lastRxFrames) ? (currentFrames - m_lastRxFrames) : 0;
                    const qint64 deltaMs = nowMs - m_lastRxSampleMs;
                    realtimeFps = (deltaMs > 0)
                        ? (static_cast<double>(deltaFrames) * 1000.0 / static_cast<double>(deltaMs))
                        : 0.0;
                }

                if (m_lastRxDevice != devices.first()) {
                    m_smoothedRxFps = -1.0;
                }
                constexpr double kAlpha = 0.25; // EMA平滑系数：越小越平稳
                if (m_smoothedRxFps < 0.0) {
                    m_smoothedRxFps = realtimeFps;
                } else {
                    m_smoothedRxFps = (kAlpha * realtimeFps) + ((1.0 - kAlpha) * m_smoothedRxFps);
                }

                m_lastRxDevice = devices.first();
                m_lastRxFrames = currentFrames;
                m_lastRxSampleMs = nowMs;
                m_throughputLabel->setText(QString("%1 帧/秒").arg(m_smoothedRxFps, 0, 'f', 0));
                
                // 更新延迟
                double latency = stats->getAvgLatencyUs();
                if (latency > 0) {
                    m_latencyLabel->setText(QString("延迟: %1μs").arg(latency, 0, 'f', 1));
                } else {
                    m_latencyLabel->setText("延迟: --");
                }
            } else {
                m_protocolModeLabel->setText("协议: --");
                m_throughputLabel->setText("0 帧/秒");
                m_latencyLabel->setText("延迟: --");
            }
        }
    }
    
    FrameStorageEngine *storage = FrameStorageEngine::instance();
    m_memoryLabel->setText(QString("存储队列: %1帧/%2批 丢弃:%3")
        .arg(storage->pendingFrameCount())
        .arg(storage->pendingBatchCount())
        .arg(storage->droppedFrameCount()));
    
    // 注意：每个子窗口有自己的统计，不需要在主窗口汇总
}

void MainWindow::loadSettings()
{
    QSettings settings("CANTools", "CANAnalyzerPro");
    
    // 不恢复窗口几何位置，始终使用默认大小和居中位置
    // 这样可以确保窗口始终可见，不会出现在屏幕外或最大化
    
    qDebug() << "📖 加载设置（跳过窗口几何恢复，避免启动问题）";
}

void MainWindow::saveSettings()
{
    QSettings settings("CANTools", "CANAnalyzerPro");
    
    // 保存窗口大小和位置
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

// ==================== 边框检测和调整大小 ====================

MainWindow::ResizeEdge MainWindow::getResizeEdge(const QPoint &pos)
{
    int edge = None;
    
    if (pos.x() <= m_borderWidth) {
        edge |= Left;
    } else if (pos.x() >= width() - m_borderWidth) {
        edge |= Right;
    }
    
    if (pos.y() <= m_borderWidth) {
        edge |= Top;
    } else if (pos.y() >= height() - m_borderWidth) {
        edge |= Bottom;
    }
    
    return static_cast<ResizeEdge>(edge);
}

void MainWindow::updateCursor(ResizeEdge edge)
{
    switch (edge) {
        case TopLeft:
        case BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight:
        case BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case Top:
        case Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case Left:
        case Right:
            setCursor(Qt::SizeHorCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
}

bool MainWindow::event(QEvent *event)
{
    // 处理Hover事件，用于更新光标
    if (event->type() == QEvent::HoverMove) {
        QHoverEvent *hoverEvent = static_cast<QHoverEvent*>(event);
        
        // 只在非拖动、非调整大小状态下更新光标
        if (!m_dragging && !m_resizing) {
            ResizeEdge edge = getResizeEdge(hoverEvent->position().toPoint());
            if (edge != None) {
                updateCursor(edge);
            } else {
                unsetCursor();
            }
        }
    }
    
    return QMainWindow::event(event);
}

// ==================== 窗口拖动和调整大小实现 ====================

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 检查是否在边框上（用于调整大小）
        m_resizeEdge = getResizeEdge(event->pos());
        if (m_resizeEdge != None) {
            m_resizing = true;
            m_resizeStartGeometry = geometry();
            m_resizeStartPos = event->globalPosition().toPoint();
            event->accept();
            return;
        }
        
        // 只在标题栏区域允许拖动（前60像素高度）
        if (event->pos().y() <= 60) {
            m_dragging = true;
            m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // 如果正在调整大小
    if (m_resizing && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->globalPosition().toPoint() - m_resizeStartPos;
        QRect newGeometry = m_resizeStartGeometry;
        
        // 根据边缘调整几何位置
        if (m_resizeEdge & Left) {
            newGeometry.setLeft(m_resizeStartGeometry.left() + delta.x());
        }
        if (m_resizeEdge & Right) {
            newGeometry.setRight(m_resizeStartGeometry.right() + delta.x());
        }
        if (m_resizeEdge & Top) {
            newGeometry.setTop(m_resizeStartGeometry.top() + delta.y());
        }
        if (m_resizeEdge & Bottom) {
            newGeometry.setBottom(m_resizeStartGeometry.bottom() + delta.y());
        }
        
        // 限制最小尺寸
        if (newGeometry.width() < 800) {
            if (m_resizeEdge & Left) {
                newGeometry.setLeft(m_resizeStartGeometry.right() - 800);
            } else {
                newGeometry.setRight(m_resizeStartGeometry.left() + 800);
            }
        }
        if (newGeometry.height() < 600) {
            if (m_resizeEdge & Top) {
                newGeometry.setTop(m_resizeStartGeometry.bottom() - 600);
            } else {
                newGeometry.setBottom(m_resizeStartGeometry.top() + 600);
            }
        }
        
        setGeometry(newGeometry);
        event->accept();
        return;
    }
    
    // 如果正在拖动标题栏
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
        return;
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        m_resizing = false;
        m_resizeEdge = None;
        event->accept();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // 启动延迟定时器，避免频繁重排
    // 只有在定时器超时后才真正重排子窗口
    m_resizeTimer->start(150);  // 150ms延迟
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    
    // 监听窗口状态变化
    if (event->type() == QEvent::WindowStateChange) {
        // 更新最大化按钮图标
        if (isMaximized()) {
            m_btnMaximize->setText("❐");  // 还原图标
            m_btnMaximize->setToolTip("还原");
        } else {
            m_btnMaximize->setText("□");  // 最大化图标
            m_btnMaximize->setToolTip("最大化");
        }
    }
}


// ==================== 辅助函数 ====================

void MainWindow::updateAllWindowsDeviceList()
{
    // 遍历所有MDI子窗口，更新设备列表
    QList<QMdiSubWindow*> subWindows = m_mdiArea->subWindowList();
    for (QMdiSubWindow *subWindow : subWindows) {
        // 使用CustomMdiSubWindow获取真正的内容widget
        CustomMdiSubWindow *customWin = qobject_cast<CustomMdiSubWindow*>(subWindow);
        if (customWin) {
            CANViewWidget *canView = qobject_cast<CANViewWidget*>(customWin->contentWidget());
            if (canView) {
                canView->updateDeviceList();
            }
        }
    }
    
    qDebug() << "✅ 已更新" << subWindows.count() << "个窗口的设备列表";
}

void MainWindow::onSubWindowClosed()
{
    qDebug() << "🗑 子窗口已关闭，重新排列剩余窗口";
    // 延迟执行，确保窗口已经从列表中移除
    QTimer::singleShot(100, this, &MainWindow::arrangeSubWindows);
}

void MainWindow::arrangeSubWindows()
{
    QList<QMdiSubWindow*> allWindows = m_mdiArea->subWindowList();
    
    // 过滤出可见的、非最小化的窗口
    QList<QMdiSubWindow*> windows;
    for (QMdiSubWindow *subWin : allWindows) {
        // 跳过不可见的窗口（正在关闭的窗口）
        if (!subWin || !subWin->isVisible()) {
            continue;
        }
        
        // 跳过最小化的窗口
        CustomMdiSubWindow *customWin = qobject_cast<CustomMdiSubWindow*>(subWin);
        if (customWin && customWin->isMinimized()) {
            continue;
        }
        
        windows.append(subWin);
    }
    
    int count = windows.size();
    
    if (count == 0) {
        qDebug() << "📐 没有需要排列的窗口（所有窗口都已最小化或不可见）";
        return;
    }
    
    QRect mdiRect = m_mdiArea->viewport()->rect();
    int mdiWidth = mdiRect.width();
    int mdiHeight = mdiRect.height();
    
    // 窗口之间的间隙（分割线）
    const int gap = 2;
    
    qDebug() << "📐 自动排列" << count << "个窗口（忽略最小化和不可见窗口），MDI区域:" << mdiWidth << "x" << mdiHeight;
    
    if (count == 1) {
        // 1个窗口：填满整个MDI区域
        windows[0]->setGeometry(0, 0, mdiWidth, mdiHeight);
        qDebug() << "   窗口1: 填满整个区域";
    }
    else if (count == 2) {
        // 2个窗口：左右平分，中间留间隙
        int halfWidth = (mdiWidth - gap) / 2;
        windows[0]->setGeometry(0, 0, halfWidth, mdiHeight);
        windows[1]->setGeometry(halfWidth + gap, 0, halfWidth, mdiHeight);
        qDebug() << "   窗口1: 左半部分，窗口2: 右半部分（间隙:" << gap << "px）";
    }
    else if (count == 3) {
        // 3个窗口：左边1个占一半，右边2个上下平分，都有间隙
        int halfWidth = (mdiWidth - gap) / 2;
        int halfHeight = (mdiHeight - gap) / 2;
        windows[0]->setGeometry(0, 0, halfWidth, mdiHeight);
        windows[1]->setGeometry(halfWidth + gap, 0, halfWidth, halfHeight);
        windows[2]->setGeometry(halfWidth + gap, halfHeight + gap, halfWidth, halfHeight);
        qDebug() << "   窗口1: 左半部分，窗口2: 右上，窗口3: 右下（间隙:" << gap << "px）";
    }
    else if (count == 4) {
        // 4个窗口：2×2网格，都有间隙
        int halfWidth = (mdiWidth - gap) / 2;
        int halfHeight = (mdiHeight - gap) / 2;
        windows[0]->setGeometry(0, 0, halfWidth, halfHeight);
        windows[1]->setGeometry(halfWidth + gap, 0, halfWidth, halfHeight);
        windows[2]->setGeometry(0, halfHeight + gap, halfWidth, halfHeight);
        windows[3]->setGeometry(halfWidth + gap, halfHeight + gap, halfWidth, halfHeight);
        qDebug() << "   2×2网格排列（间隙:" << gap << "px）";
    }
    else {
        // 5个及以上：层叠排列（不需要间隙）
        int offsetX = 30;
        int offsetY = 30;
        int windowWidth = mdiWidth - (count - 1) * offsetX;
        int windowHeight = mdiHeight - (count - 1) * offsetY;
        
        // 确保窗口不会太小
        if (windowWidth < 400) windowWidth = 400;
        if (windowHeight < 300) windowHeight = 300;
        
        for (int i = 0; i < count; i++) {
            windows[i]->setGeometry(
                i * offsetX,
                i * offsetY,
                windowWidth,
                windowHeight
            );
        }
        qDebug() << "   层叠排列，偏移:" << offsetX << "x" << offsetY;
    }
    
    // 激活最后一个窗口
    if (!windows.isEmpty()) {
        m_mdiArea->setActiveSubWindow(windows.last());
    }
    
    // 确保所有最小化窗口仍然在最上层
    for (QMdiSubWindow *subWin : allWindows) {
        if (!subWin || !subWin->isVisible()) {
            continue;
        }
        CustomMdiSubWindow *customWin = qobject_cast<CustomMdiSubWindow*>(subWin);
        if (customWin && customWin->isMinimized()) {
            customWin->raise();
        }
    }
}

// ==================== 加载应用程序样式表 ====================

void MainWindow::loadStyleSheet()
{
    // 从Qt资源加载样式表
    QFile styleFile(":/styles/app_styles.qss");
    if (!styleFile.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "❌ 无法加载样式表文件: app_styles.qss";
        return;
    }
    
    // 读取样式表内容
    QTextStream in(&styleFile);
    QString styleSheet = in.readAll();
    styleFile.close();
    
    // 替换%IMAGE_DIR%占位符为Qt资源路径
    styleSheet.replace("%IMAGE_DIR%", ":/icons");
    
    // 应用样式表到整个应用程序
    qobject_cast<QApplication*>(QApplication::instance())->setStyleSheet(styleSheet);
    
    qDebug() << "✅ 已加载应用程序样式表（icons文件夹图标已应用）";
}
