#include "LogManager.h"
#include <cstdio>

std::atomic_bool LogManager::s_enabled{false};
std::atomic_int LogManager::s_minLevel{(int)LogLevel::INFO};

LogManager* LogManager::instance()
{
    static LogManager instance;
    return &instance;
}

void LogManager::setEnabled(bool enabled)
{
    s_enabled.store(enabled, std::memory_order_relaxed);
    emit instance()->enabledChanged(enabled);
}

bool LogManager::isEnabled()
{
    return s_enabled.load(std::memory_order_relaxed);
}

void LogManager::setMinLevel(LogLevel level)
{
    s_minLevel.store((int)level, std::memory_order_relaxed);
    emit instance()->minLevelChanged(level);
}

LogLevel LogManager::getMinLevel()
{
    return (LogLevel)s_minLevel.load(std::memory_order_relaxed);
}

LogManager::LogManager(QObject *parent)
    : QObject(parent)
{
}

LogManager::~LogManager()
{
}

void LogManager::log(LogLevel level, LogCategory category, const QString& message)
{
    QMutexLocker locker(&m_mutex);

    QString logMsg = QString("[%1] [%2] [%3] %4")
        .arg(getTimestamp())
        .arg(levelToString(level))
        .arg(categoryToString(category))
        .arg(message);

    std::fprintf(stderr, "%s\n", logMsg.toLocal8Bit().constData());
    std::fflush(stderr);

    emit logMessage(logMsg);
}

QString LogManager::levelToString(LogLevel level) const
{
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

QString LogManager::categoryToString(LogCategory category) const
{
    switch (category) {
        case LogCategory::ZCAN: return "ZCAN";
        case LogCategory::UI: return "UI";
        case LogCategory::NETWORK: return "NET";
        case LogCategory::DEVICE: return "DEV";
        case LogCategory::GENERAL: return "GEN";
        default: return "UNKNOWN";
    }
}

QString LogManager::getTimestamp() const
{
    return QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
}
