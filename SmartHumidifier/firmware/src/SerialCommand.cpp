/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * SerialCommand - 串口命令解析器（实现文件）
 * ============================================================
 */

#include "SerialCommand.h"
#include "Logger.h"

// ============================================================
// 初始化
// ============================================================

void SerialCommand::begin() {
    _commandCount = 0;
    _bufferIndex = 0;
    memset(_buffer, 0, CMD_BUFFER_SIZE);

    LOG_INFO("CMD", "Serial command parser ready");
    LOG_INFO("CMD", "Type HELP for available commands");
}

// ============================================================
// 注册命令
// ============================================================

void SerialCommand::addCommand(const char* name, const char* helpText,
                                void (*handler)(const char* args)) {
    if (_commandCount < MAX_COMMANDS) {
        _commands[_commandCount].name = name;
        _commands[_commandCount].helpText = helpText;
        _commands[_commandCount].handler = handler;
        _commandCount++;
        LOG_DEBUG("CMD", ("Registered: " + String(name)).c_str());
    } else {
        LOG_ERROR("CMD", "Too many commands! Increase MAX_COMMANDS.");
    }
}

// ============================================================
// 主循环更新 — 这是你在 loop() 里每次都要调用的
// ============================================================

void SerialCommand::update() {
    // 1. 检查串口有没有新数据
    while (Serial.available() > 0) {
        char c = Serial.read();

        // 2. 遇到换行符 → 这一行输入完毕，开始解析
        //    Windows 发送 \r\n，所以同时处理两者
        if (c == '\n' || c == '\r') {
            if (_bufferIndex > 0) {
                _buffer[_bufferIndex] = '\0';  // 字符串结束符
                _parseAndExecute();
                _bufferIndex = 0;              // 重置缓冲区

                // 显示新提示符
                Serial.print(F("> "));
            }
        }
        // 3. 退格键 → 删掉最后一个字符
        else if (c == '\b' || c == 127) {
            if (_bufferIndex > 0) {
                _bufferIndex--;
                _buffer[_bufferIndex] = '\0';
            }
        }
        // 4. 普通字符 → 放入缓冲区
        else if (_bufferIndex < CMD_BUFFER_SIZE - 1) {
            // 只接受可打印字符
            if (c >= 32 && c <= 126) {
                _buffer[_bufferIndex] = c;
                _bufferIndex++;
            }
        }
    }
}

// ============================================================
// 打印帮助
// ============================================================

void SerialCommand::printHelp() {
    Serial.println();
    Serial.println(F("Available commands:"));
    Serial.println(F("----------------------------------------"));

    for (int i = 0; i < _commandCount; i++) {
        // 格式：STATUS    - Show system status
        Serial.print(F("  "));
        Serial.print(_commands[i].name);

        // 补空格对齐（最长命令名 8 个字符）
        int padding = 10 - strlen(_commands[i].name);
        while (padding-- > 0) Serial.print(F(" "));

        Serial.print(F("- "));
        Serial.println(_commands[i].helpText);
    }

    Serial.println(F("----------------------------------------"));
    Serial.println();
}

// ============================================================
// 私有方法
// ============================================================

void SerialCommand::_parseAndExecute() {
    _trim(_buffer);

    // 空行忽略
    if (strlen(_buffer) == 0) return;

    // 分离命令名和参数
    // 格式：COMMAND arg1 arg2
    //        ↑         ↑
    //     命令名      参数（可选）
    char commandName[CMD_BUFFER_SIZE];
    char args[CMD_BUFFER_SIZE];
    memset(commandName, 0, CMD_BUFFER_SIZE);
    memset(args, 0, CMD_BUFFER_SIZE);

    // 找第一个空格的位置
    char* spacePos = strchr(_buffer, ' ');
    if (spacePos != NULL) {
        // 有参数：把空格前复制到 commandName，空格后复制到 args
        int nameLen = spacePos - _buffer;
        strncpy(commandName, _buffer, nameLen);
        strncpy(args, spacePos + 1, CMD_BUFFER_SIZE - 1);
    } else {
        // 没有参数：整行都是命令名
        strncpy(commandName, _buffer, CMD_BUFFER_SIZE - 1);
    }

    _toUpperCase(commandName);

    // 在命令表中查找匹配的命令
    for (int i = 0; i < _commandCount; i++) {
        char registeredName[32];
        strncpy(registeredName, _commands[i].name, 31);
        _toUpperCase(registeredName);

        if (strcmp(commandName, registeredName) == 0) {
            // 找到了，执行回调函数
            _commands[i].handler(args);
            return;
        }
    }

    // 没找到这个命令
    Serial.print(F("Unknown command: "));
    Serial.println(_buffer);
    Serial.println(F("Type HELP to see available commands."));
}

void SerialCommand::_trim(char* str) {
    // 去除尾部空白
    int end = strlen(str) - 1;
    while (end >= 0 && (str[end] == ' ' || str[end] == '\r' || str[end] == '\n')) {
        str[end] = '\0';
        end--;
    }

    // 去除头部空白
    int start = 0;
    while (str[start] == ' ') {
        start++;
    }

    // 把去掉头部空白后的字符串移到开头
    if (start > 0) {
        int i = 0;
        while (str[start + i] != '\0') {
            str[i] = str[start + i];
            i++;
        }
        str[i] = '\0';
    }
}

void SerialCommand::_toUpperCase(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = str[i] - 32;  // ASCII: 'a' - 32 = 'A'
        }
    }
}
