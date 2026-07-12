/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * ButtonManager - 按钮管理模块（头文件）
 * ============================================================
 *
 * 职责：检测两个按钮的按下事件，支持单击/双击/长按/超长按。
 *
 * 按钮 A (GPIO13)：模式切换
 *   单击    → 切换 OLED 页面
 *   双击    → 手动开关继电器（调试用）
 *   长按1秒 → 切换手动/自动模式
 *
 * 按钮 B (GPIO14)：系统控制
 *   单击    → 恢复默认阈值
 *   长按3秒 → 静音/取消静音蜂鸣器
 *   超长按5秒 → 恢复出厂设置
 *
 * 消抖策略：
 *   - 使用 millis() 计时，非阻塞
 *   - 状态变化后锁定 30ms（消抖窗口）
 *   - 松开后 400ms 内再次按下 = 双击
 */

#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>

// 按钮事件类型
enum class ButtonEvent {
    NONE,               // 无事件
    PRESSED,            // 刚刚按下（瞬时）
    RELEASED,           // 刚刚松开（瞬时）
    CLICK,              // 单击（按下 + 松开，<1秒）
    DOUBLE_CLICK,       // 双击（两次单击间隔 <400ms）
    LONG_PRESS,         // 长按（按下超过 1 秒）
    VERY_LONG_PRESS     // 超长按（按下超过 3 秒）
};

class ButtonManager {
public:
    /**
     * 初始化两个按钮
     * @param pinA  按钮 A 引脚
     * @param pinB  按钮 B 引脚
     */
    void begin(uint8_t pinA, uint8_t pinB);

    /**
     * 每循环调用一次（每 ~10ms 调用效果最好）
     * 检测按钮状态变化，生成事件
     */
    void update();

    // ---- 事件查询（读取后自动清除）----

    /** 获取按钮 A 的最新事件 */
    ButtonEvent getEventA();

    /** 获取按钮 B 的最新事件 */
    ButtonEvent getEventB();

    // ---- 实时状态（不消耗事件）----

    /** 按钮 A 当前是否被按住 */
    bool isPressedA();

    /** 按钮 B 当前是否被按住 */
    bool isPressedB();

    /** 按钮 A 已经按住了多少毫秒（松开时返回 0） */
    unsigned long pressDurationA();

    /** 按钮 B 已经按住了多少毫秒 */
    unsigned long pressDurationB();

private:
    // 单个按钮的状态机
    struct ButtonState {
        uint8_t pin;

        // 物理状态
        bool reading;         // 本次读取的电平
        bool lastReading;     // 上次读取的电平
        bool pressed;         // 当前是否处于按下状态
        bool debounced;       // 消抖确认后的稳定状态

        // 计时
        unsigned long pressStart;     // 按下开始时刻
        unsigned long releaseTime;    // 上次松开时刻
        unsigned long lastDebounceTime; // 上次电平变化时刻

        // 事件检测
        bool waitingForDouble;  // 正在等待可能的第二次点击
        ButtonEvent pendingEvent;   // 待消费的事件
        bool eventConsumed;         // 事件是否已被读取
    };

    ButtonState _btnA;
    ButtonState _btnB;

    void _initButton(ButtonState& btn, uint8_t pin);
    void _updateButton(ButtonState& btn);
    void _detectEvents(ButtonState& btn);
};

#endif // BUTTON_MANAGER_H
