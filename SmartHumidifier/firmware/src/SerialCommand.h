/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * SerialCommand - 串口命令解析器（头文件）
 * ============================================================
 *
 * 功能：从串口读取用户输入的命令，解析并执行。
 *
 * 支持的命令（V1.0）：
 *   STATUS  - 显示系统运行状态
 *   RESET   - 软件重启设备
 *   HELP    - 显示所有可用命令
 *   INFO    - 显示设备信息
 *
 * 用法：
 *   SerialCommand cmd;
 *   cmd.begin();           // 在 setup() 中初始化
 *   cmd.update();          // 在 loop() 中每循环调用一次
 *
 * 设计原则：
 *   - 非阻塞：读取串口数据有就处理，没有就立刻返回
 *   - 大小写不敏感：STATUS = status = Status
 *   - 忽略空白字符：用户多打了空格不影响解析
 */

#ifndef SERIAL_COMMAND_H
#define SERIAL_COMMAND_H

#include <Arduino.h>
#include "config.h"

// 命令缓冲区大小（用户输入的每一行最大长度）
#define CMD_BUFFER_SIZE  64

// 支持的最大命令数量
#define MAX_COMMANDS     16

class SerialCommand {
public:
    /**
     * 初始化命令解析器
     */
    void begin();

    /**
     * 每循环调用一次，检查串口是否有新输入
     * 有完整的一行就解析执行，没有就立刻返回
     */
    void update();

    /**
     * 注册一个命令
     * @param name     命令名（大小写不敏感），如 "STATUS"
     * @param helpText 帮助说明，如 "Show system status"
     * @param handler  回调函数，执行命令时调用
     */
    void addCommand(const char* name, const char* helpText,
                    void (*handler)(const char* args));

    /**
     * 打印所有已注册的命令列表
     */
    void printHelp();

private:
    // 命令结构体：名字 + 帮助文本 + 回调函数
    struct Command {
        const char* name;
        const char* helpText;
        void (*handler)(const char* args);
    };

    Command _commands[MAX_COMMANDS];  // 所有注册的命令
    int _commandCount;                // 当前注册了几个命令
    char _buffer[CMD_BUFFER_SIZE];    // 输入缓冲区
    int _bufferIndex;                 // 缓冲区当前写入位置

    /**
     * 从缓冲区解析并执行命令
     */
    void _parseAndExecute();

    /**
     * 去除字符串首尾的空白字符（空格、\r、\n）
     */
    void _trim(char* str);

    /**
     * 将字符串转为大写（用于大小写不敏感比较）
     */
    void _toUpperCase(char* str);
};

#endif // SERIAL_COMMAND_H
