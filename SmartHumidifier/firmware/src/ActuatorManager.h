/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * ActuatorManager - 执行器管理模块（头文件）
 * ============================================================
 *
 * 职责：管理所有"输出"设备
 *   - RelayManager  ：继电器（控制加湿器电源）
 *   - LedManager    ：RGB LED 三色状态指示
 *   - BuzzerManager ：蜂鸣器报警
 *
 * 设计原则：
 *   - 继电器默认关闭（Fail-Safe：不确定时切断高压最安全）
 *   - LED/LED 状态由系统状态驱动，不独立决策
 *   - 蜂鸣器非阻塞（不能因为响而卡住主循环）
 */

#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <Arduino.h>

// ============================================================
// LED 颜色枚举
// ============================================================

enum class LedColor {
    OFF,
    GREEN,    // 正常运行
    BLUE,     // 正在加湿
    RED,      // 传感器/系统异常
    YELLOW,   // 警告（WiFi 断开等）
    COLOR_WHITE  // 初始化中（避免与SSD1306库的WHITE宏冲突）
};

enum class LedPattern {
    SOLID,        // 常亮
    SLOW_BLINK,   // 慢闪（1 秒周期）
    FAST_BLINK,   // 快闪（0.3 秒周期）
    DOUBLE_BLINK  // 双闪
};

// ============================================================
// 继电器管理
// ============================================================

class RelayManager {
public:
    /** 初始化，设置引脚和触发方式 */
    void begin(uint8_t pin, bool activeHigh);

    /** 开启继电器（闭合触点） */
    void turnOn();

    /** 关闭继电器（断开触点） */
    void turnOff();

    /** 切换状态 */
    void toggle();

    /** 当前是否开启 */
    bool isOn();

    /** 距离上次状态变化过了多少毫秒 */
    unsigned long timeSinceLastChange();

    /** 总开启次数（调试用） */
    unsigned long getCycleCount();

private:
    uint8_t _pin;
    bool _activeHigh;        // true=高电平触发, false=低电平触发
    bool _state;             // 当前状态
    unsigned long _lastChangeTime;
    unsigned long _cycleCount;

    void _applyState();      // 把逻辑状态转换为实际的 GPIO 电平
};

// ============================================================
// LED 管理（非阻塞闪烁）
// ============================================================

class LedManager {
public:
    void begin(uint8_t pinR, uint8_t pinG, uint8_t pinB);

    /** 设置颜色和闪烁模式 */
    void set(LedColor color, LedPattern pattern = LedPattern::SOLID);

    /** 关闭所有 LED */
    void turnOff();

    /**
     * 每循环调用一次，处理闪烁定时
     * 必须放在 loop() 中高频调用
     */
    void update();

    /** 获取当前颜色（调试用） */
    LedColor getColor();

private:
    uint8_t _pinR, _pinG, _pinB;
    LedColor _color;
    LedPattern _pattern;
    bool _visible;            // 当前闪烁相位：显示/隐藏
    unsigned long _lastToggleTime;
    int _blinkStep;

    void _writeRGB(bool r, bool g, bool b);
};

// ============================================================
// 蜂鸣器管理（非阻塞报警）
// ============================================================

class BuzzerManager {
public:
    void begin(uint8_t pin);

    /** 短促提示音 */
    void beep();

    /** 双短促提示音 */
    void doubleBeep();

    /** 报警模式 */
    void alarmSensorError();   // 传感器故障：间歇长鸣
    void alarmWifiLost();      // WiFi 断开：三短一长
    void alarmCritical();      // 严重故障：连续急促

    /** 停止所有声音 */
    void stop();

    /** 静音切换 */
    void mute();
    void unmute();
    bool isMuted();

    /** 每循环调用 */
    void update();

private:
    uint8_t _pin;
    bool _active;
    bool _muted;
    int _pattern;              // 0=无, 1=传感器, 2=WiFi, 3=严重
    int _step;
    unsigned long _lastToggleTime;
    bool _buzzerState;

    void _setPin(bool on);
};

#endif // ACTUATOR_MANAGER_H
