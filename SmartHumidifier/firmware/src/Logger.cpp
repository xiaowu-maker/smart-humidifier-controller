/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * Logger - 统一日志系统（实现文件）
 * ============================================================
 */

#include "Logger.h"
#include "config.h"

// ============================================================
// 公共方法
// ============================================================

void Logger::log(LogLevel level, const char* tag, const char* message) {
    // 格式：[LEVEL] [TAG] message
    Serial.print(F("["));
    Serial.print(levelToString(level));
    Serial.print(F("] "));

    // 标签固定宽度（7 个字符），右对齐，不足补空格
    // 这样不同模块的日志对齐整齐
    Serial.print(F("["));
    Serial.print(tag);

    // 计算需要补几个空格
    int tagLen = strlen(tag);
    int padding = 7 - tagLen;
    while (padding > 0) {
        Serial.print(F(" "));
        padding--;
    }
    Serial.print(F("] "));

    Serial.println(message);
}

void Logger::printBanner() {
    printSeparator();
    Serial.print(F("  "));
    Serial.println(PROJECT_NAME);
    Serial.print(F("  Version: "));
    Serial.println(PROJECT_VERSION);
    printSeparator();
    Serial.print(F("  MCU   : ESP32 DevKit V1"));
    Serial.println();
    Serial.print(F("  Flash : 4MB"));
    Serial.println();
    Serial.print(F("  Freq  : 240MHz"));
    Serial.println();
    printSeparator();
}

void Logger::printSeparator() {
    Serial.println(F("========================================"));
}

// ============================================================
// 私有方法
// ============================================================

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LEVEL_DEBUG: return "DEBUG";
        case LEVEL_INFO:  return "INFO ";
        case LEVEL_WARN:  return "WARN ";
        case LEVEL_ERROR: return "ERROR";
        default:          return "?????";
    }
}
