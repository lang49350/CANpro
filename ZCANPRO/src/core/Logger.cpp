/**
 * @file Logger.cpp
 * @brief 日志管理系统实现
 * @author CANAnalyzerPro Team
 * @date 2024-02-10
 */

#include "Logger.h"

// 初始化静态成员
LogLevel Logger::s_level = LogLevel::Debug;
