QT += core gui widgets serialport sql

CONFIG += c++17

# 项目名称
TARGET = CANAnalyzerPro
TEMPLATE = app

# 定义
DEFINES += QT_DEPRECATED_WARNINGS
# 关闭Qt原生日志宏输出，避免窗口关闭时产生额外日志开销
DEFINES += QT_NO_DEBUG_OUTPUT QT_NO_INFO_OUTPUT QT_NO_WARNING_OUTPUT

# 源文件
SOURCES += \
    src/main.cpp \
    src/ui/MainWindow.cpp \
    src/ui/CANViewWidget.cpp \
    src/ui/CANViewSettingsDialog.cpp \
    src/ui/CANViewRealTimeSaver.cpp \
    src/ui/CANFrameDetailPanel.cpp \
    src/ui/CustomMdiSubWindow.cpp \
    src/ui/CustomComboBox.cpp \
    src/ui/DeviceManagerDialog.cpp \
    src/ui/DebugLogWindow.cpp \
    src/ui/AnimatedToolButton.cpp \
    src/ui/StyleManager.cpp \
    src/ui/FilterHeaderWidget.cpp \
    src/ui/HighlightDelegate.cpp \
    src/ui/StatisticsDialog.cpp \
    src/ui/ProtocolConfigDialog.cpp \
    src/ui/FramePlaybackWidget.cpp \
    src/ui/FrameSenderWidget.cpp \
    src/core/CANFrame.cpp \
    src/core/ParserThread.cpp \
    src/core/SerialConnection.cpp \
    src/core/CANFrameTableModel.cpp \
    src/core/ConnectionManager.cpp \
    src/core/DataRouter.cpp \
    src/core/IDFilterMatcher.cpp \
    src/core/Logger.cpp \
    src/core/LogManager.cpp \
    src/core/FilterManager.cpp \
    src/core/SimpleXlsxWriter.cpp \
    src/core/FramePlaybackCore.cpp \
    src/core/PlaybackThread.cpp \
    src/core/FrameFileParser.cpp \
    src/core/FrameSenderCore.cpp \
    src/core/FrameStorageEngine.cpp \
    src/core/blf/BLFHandler.cpp \
    src/core/zcan/ZCANFramePool.cpp \
    src/core/zcan/ZCANBatchParser.cpp \
    src/core/zcan/ZCANStatistics.cpp

# 头文件
HEADERS += \
    src/ui/MainWindow.h \
    src/ui/CANViewWidget.h \
    src/ui/CANViewSettingsDialog.h \
    src/ui/CANViewRealTimeSaver.h \
    src/ui/CANFrameDetailPanel.h \
    src/ui/CustomMdiSubWindow.h \
    src/ui/CustomComboBox.h \
    src/ui/DeviceManagerDialog.h \
    src/ui/DebugLogWindow.h \
    src/ui/AnimatedToolButton.h \
    src/ui/StyleManager.h \
    src/ui/FilterHeaderWidget.h \
    src/ui/HighlightDelegate.h \
    src/ui/StatisticsDialog.h \
    src/ui/ProtocolConfigDialog.h \
    src/ui/FramePlaybackWidget.h \
    src/ui/FrameSenderWidget.h \
    src/core/CANFrame.h \
    src/core/ParserThread.h \
    src/core/SerialConnection.h \
    src/core/CANFrameTableModel.h \
    src/core/ConnectionManager.h \
    src/core/DataRouter.h \
    src/core/IDFilterRule.h \
    src/core/IDFilterMatcher.h \
    src/core/Logger.h \
    src/core/LogManager.h \
    src/core/FilterManager.h \
    src/core/SimpleXlsxWriter.h \
    src/core/FramePlaybackCore.h \
    src/core/PlaybackThread.h \
    src/core/FrameProvider.h \
    src/core/FrameFileParser.h \
    src/core/FrameSenderCore.h \
    src/core/FrameStorageEngine.h \
    src/core/blf/BLFHandler.h \
    src/core/zcan/zcan_types.h \
    src/core/zcan/zcan_config.h \
    src/core/zcan/ZCANFramePool.h \
    src/core/zcan/ZCANBatchParser.h \
    src/core/zcan/ZCANStatistics.h

# UI文件
# FORMS += \
#     src/ui/MainWindow.ui  # 暂时不使用.ui文件，使用纯代码创建UI

# 资源文件
RESOURCES += \
    resources/resources.qrc

# 输出目录
DESTDIR = bin
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui

# Windows特定配置
win32 {
    RC_FILE = resources/app.rc  # 启用Windows资源文件，包含图标
}

# 默认部署规则
qnx: target.path = /tmp/${TARGET}/bin
else: unix:!android: target.path = /opt/${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
