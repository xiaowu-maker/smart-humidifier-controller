/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * Logger - 统一日志系统（头文件）
 * ============================================================
 *
 * 功能：提供统一的日志输出接口
 *
 * 日志级别（从低到高）：
 *   DEBUG (0) - 调试信息，开发时使用
 *   INFO  (1) - 一般运行信息
 *   WARN  (2) - 警告，需要关注但不影响运行
 *   ERROR (3) - 错误，功能受影响
 *
 * 输出格式：[LEVEL] [TAG] message
 * 示例：   [INFO] [SYSTEM] Smart Humidifier Controller V1.0
 *
 * 用法：
 *   LOG_INFO("SYSTEM", "System booting...");
 *   LOG_ERROR("SENSOR", "Sensor disconnected!");
 *
 * 全局日志级别通过 LOG_LEVEL 宏控制，低于该级别的日志不会输出。
 * 例如 LOG_LEVEL=2，则 DEBUG 和 INFO 都不输出，只输出 WARN 和 ERROR。
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// ============================================================
// 日志级别枚举
// ============================================================

enum LogLevel {
    LEVEL_DEBUG = 0,  // 调试：最啰嗦，开发阶段用
    LEVEL_INFO  = 1,  // 信息：正常运行的关键节点
    LEVEL_WARN  = 2,  // 警告：不太对劲但不致命
    LEVEL_ERROR = 3   // 错误：功能出问题了
};

// ============================================================
// 宏定义函数（比普通函数调用更快，不占栈空间）
// ============================================================

/**
 * 这些宏函数在编译时会检查 LOG_LEVEL：
 * - 如果日志级别不够，整行代码都不会编译进去
 * - 也就是说，开发完成后把 LOG_LEVEL 调高，
 *   调试日志不仅不输出，甚至不占 Flash 空间。
 */

#if LOG_LEVEL <= 0
  #define LOG_DEBUG(tag, msg)   Logger::log(LEVEL_DEBUG, tag, msg)
#else
  #define LOG_DEBUG(tag, msg)   // 空操作，编译器会直接删除
#endif

#if LOG_LEVEL <= 1
  #define LOG_INFO(tag, msg)    Logger::log(LEVEL_INFO, tag, msg)
#else
  #define LOG_INFO(tag, msg)
#endif

#if LOG_LEVEL <= 2
  #define LOG_WARN(tag, msg)    Logger::log(LEVEL_WARN, tag, msg)
#else
  #define LOG_WARN(tag, msg)
#endif

#if LOG_LEVEL <= 3
  #define LOG_ERROR(tag, msg)   Logger::log(LEVEL_ERROR, tag, msg)
#else
  #define LOG_ERROR(tag, msg)
#endif

// ============================================================
// Logger 静态类
// ============================================================

class Logger {
public:
    /**
     * 核心日志输出函数
     * @param level  日志级别
     * @param tag    模块标签（如 "SYSTEM", "SENSOR", "RELAY"）
     * @param message 日志内容
     */
    static void log(LogLevel level, const char* tag, const char* message);

    /**
     * 输出启动横幅
     * 在 setup() 开始时调用，显示项目名称和版本
     */
    static void printBanner();

    /**
     * 输出分隔线（纯视觉效果）
     */
    static void printSeparator();

private:
    /**
     * 将日志级别转换为字符串
     */
    static const char* levelToString(LogLevel level);
};

#endif // LOGGER_H
