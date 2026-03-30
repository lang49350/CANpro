/**
 * @file Logger.h
 * @brief 日志管理系统
 * @author CANAnalyzerPro Team
 * @date 2024-02-10
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QDebug>

/**
 * @brief 日志级别
 */
enum class LogLevel {
    Debug,      // 调试信息
    Info,       // 一般信息
    Warning,    // 警告信息
    Error       // 错误信息
};

/**
 * @brief 日志管理类
 * 
 * 使用条件编译控制日志输出，生产环境不输出调试日志
 */
class Logger
{
public:
    /**
     * @brief 输出调试日志
     * @param message 日志消息
     */
    static void debug(const QString &message)
    {
#ifdef QT_DEBUG
        qDebug().noquote() << "[DEBUG]" << message;
#else
        Q_UNUSED(message);
#endif
    }
    
    /**
     * @brief 输出信息日志
     * @param message 日志消息
     */
    static void info(const QString &message)
    {
        qInfo().noquote() << "[INFO]" << message;
    }
    
    /**
     * @brief 输出警告日志
     * @param message 日志消息
     */
    static void warning(const QString &message)
    {
        qWarning().noquote() << "[WARNING]" << message;
    }
    
    /**
     * @brief 输出错误日志
     * @param message 日志消息
     */
    static void error(const QString &message)
    {
        qCritical().noquote() << "[ERROR]" << message;
    }
    
    /**
     * @brief 设置日志级别
     * @param level 日志级别
     */
    static void setLevel(LogLevel level)
    {
        s_level = level;
    }
    
    /**
     * @brief 获取当前日志级别
     * @return 日志级别
     */
    static LogLevel level()
    {
        return s_level;
    }

private:
    static LogLevel s_level;
};

#endif // LOGGER_H
