#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QDebug>
#include <QDateTime>
#include <QMutex>
#include <atomic>

/**
 * @brief 高性能日志管理器
 * 
 * 特性:
 * 1. 零开销: 在Release模式下,DEBUG日志完全不执行(编译期优化)
 * 2. 线程安全: 使用互斥锁保护日志输出
 * 3. 分类管理: 支持按模块分类日志
 * 4. 运行时控制: 可动态开关日志级别
 */

// 日志级别
enum class LogLevel {
    DEBUG = 0,    // 调试信息(仅Debug模式)
    INFO = 1,     // 一般信息
    WARNING = 2,  // 警告
    ERROR = 3,    // 错误
    NONE = 4      // 禁用所有日志
};

// 日志分类
enum class LogCategory {
    ZCAN,         // ZCAN协议相关
    UI,           // UI相关
    NETWORK,      // 网络通信
    DEVICE,       // 设备管理
    GENERAL       // 通用
};

class LogManager : public QObject
{
    Q_OBJECT
    
public:
    static LogManager* instance();
    
    // 全局日志开关（最快，1纳秒）
    static void setEnabled(bool enabled);
    static bool isEnabled();
    
    // 最小日志级别（灵活过滤）
    static void setMinLevel(LogLevel level);
    static LogLevel getMinLevel();
    
    // 日志输出(内部使用)
    void log(LogLevel level, LogCategory category, const QString& message);
    
signals:
    void logMessage(const QString& message);
    void enabledChanged(bool enabled);
    void minLevelChanged(LogLevel level);
    
private:
    explicit LogManager(QObject *parent = nullptr);
    ~LogManager();
    
    static std::atomic_bool s_enabled;      // 全局开关
    static std::atomic_int s_minLevel;      // 最小级别
    QMutex m_mutex;
    
    QString levelToString(LogLevel level) const;
    QString categoryToString(LogCategory category) const;
    QString getTimestamp() const;
};

// ============================================================================
// 宏定义: 零开销日志接口
// ============================================================================

// Debug模式: 启用所有日志
// Release模式: DEBUG日志完全不编译,零开销
#ifdef QT_NO_DEBUG
    #define LOG_DEBUG(category, msg) ((void)0)
#else
    #define LOG_DEBUG(category, msg) \
        do { \
            if (LogManager::isEnabled() && LogManager::getMinLevel() <= LogLevel::DEBUG) { \
                LogManager::instance()->log(LogLevel::DEBUG, category, msg); \
            } \
        } while(0)
#endif

#define LOG_INFO(category, msg) \
    do { \
        if (LogManager::isEnabled() && LogManager::getMinLevel() <= LogLevel::INFO) { \
            LogManager::instance()->log(LogLevel::INFO, category, msg); \
        } \
    } while(0)

#define LOG_WARNING(category, msg) \
    do { \
        if (LogManager::isEnabled() && LogManager::getMinLevel() <= LogLevel::WARNING) { \
            LogManager::instance()->log(LogLevel::WARNING, category, msg); \
        } \
    } while(0)

#define LOG_ERROR(category, msg) \
    do { \
        if (LogManager::isEnabled() && LogManager::getMinLevel() <= LogLevel::ERROR) { \
            LogManager::instance()->log(LogLevel::ERROR, category, msg); \
        } \
    } while(0)

// 便捷宏: 带格式化的日志
#define LOG_DEBUG_F(category, ...) LOG_DEBUG(category, QString::asprintf(__VA_ARGS__))
#define LOG_INFO_F(category, ...) LOG_INFO(category, QString::asprintf(__VA_ARGS__))
#define LOG_WARNING_F(category, ...) LOG_WARNING(category, QString::asprintf(__VA_ARGS__))
#define LOG_ERROR_F(category, ...) LOG_ERROR(category, QString::asprintf(__VA_ARGS__))

#endif // LOGMANAGER_H
