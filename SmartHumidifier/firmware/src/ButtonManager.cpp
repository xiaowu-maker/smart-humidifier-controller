/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * ButtonManager - 按钮管理模块（实现文件）
 * ============================================================
 */

#include "ButtonManager.h"
#include "config.h"
#include "Logger.h"

// ============================================================
// 公开方法
// ============================================================

void ButtonManager::begin(uint8_t pinA, uint8_t pinB) {
    _initButton(_btnA, pinA);
    _initButton(_btnB, pinB);
    LOG_INFO("BUTTON", "Buttons ready (A=GPIO13, B=GPIO14)");
}

void ButtonManager::update() {
    _updateButton(_btnA);
    _updateButton(_btnB);
}

// ---- 事件查询 ----

ButtonEvent ButtonManager::getEventA() {
    if (!_btnA.eventConsumed) {
        _btnA.eventConsumed = true;
        return _btnA.pendingEvent;
    }
    return ButtonEvent::NONE;
}

ButtonEvent ButtonManager::getEventB() {
    if (!_btnB.eventConsumed) {
        _btnB.eventConsumed = true;
        return _btnB.pendingEvent;
    }
    return ButtonEvent::NONE;
}

// ---- 实时状态 ----

bool ButtonManager::isPressedA() {
    return _btnA.pressed;
}

bool ButtonManager::isPressedB() {
    return _btnB.pressed;
}

unsigned long ButtonManager::pressDurationA() {
    if (!_btnA.pressed) return 0;
    return millis() - _btnA.pressStart;
}

unsigned long ButtonManager::pressDurationB() {
    if (!_btnB.pressed) return 0;
    return millis() - _btnB.pressStart;
}

// ============================================================
// 内部方法
// ============================================================

void ButtonManager::_initButton(ButtonState& btn, uint8_t pin) {
    btn.pin = pin;
    btn.reading = HIGH;          // 上拉 = 未按下
    btn.lastReading = HIGH;
    btn.pressed = false;
    btn.debounced = HIGH;
    btn.pressStart = 0;
    btn.releaseTime = 0;
    btn.lastDebounceTime = 0;
    btn.waitingForDouble = false;
    btn.pendingEvent = ButtonEvent::NONE;
    btn.eventConsumed = true;

    pinMode(pin, INPUT_PULLUP);  // 内部上拉：按下=LOW，松开=HIGH
}

void ButtonManager::_updateButton(ButtonState& btn) {
    // 1. 读取当前电平
    btn.reading = digitalRead(btn.pin);

    // 2. 消抖：电平变化后需要稳定 BUTTON_DEBOUNCE_MS 毫秒才确认
    if (btn.reading != btn.lastReading) {
        btn.lastDebounceTime = millis();
    }

    if ((millis() - btn.lastDebounceTime) > BUTTON_DEBOUNCE_MS) {
        // 消抖通过，确认电平变化
        if (btn.reading != btn.debounced) {
            btn.debounced = btn.reading;

            // 按下（LOW = 按钮被按下，因为用了 INPUT_PULLUP）
            if (btn.debounced == LOW) {
                btn.pressed = true;
                btn.pressStart = millis();

                // 检查是否构成双击
                if (btn.waitingForDouble &&
                    (millis() - btn.releaseTime) < BUTTON_DOUBLE_CLICK_MS) {
                    btn.pendingEvent = ButtonEvent::DOUBLE_CLICK;
                    btn.eventConsumed = false;
                    btn.waitingForDouble = false;
                    LOG_DEBUG("BUTTON", ("Pin " + String(btn.pin) + " DOUBLE CLICK").c_str());
                } else {
                    btn.pendingEvent = ButtonEvent::PRESSED;
                    btn.eventConsumed = false;
                }
            }
            // 松开（HIGH = 按钮松开）
            else {
                btn.pressed = false;
                btn.releaseTime = millis();

                // 判断按了多久
                unsigned long duration = btn.releaseTime - btn.pressStart;

                if (duration >= BUTTON_VERY_LONG_PRESS_MS) {
                    // 超长按
                    btn.pendingEvent = ButtonEvent::VERY_LONG_PRESS;
                    btn.eventConsumed = false;
                    btn.waitingForDouble = false;
                    LOG_DEBUG("BUTTON", ("Pin " + String(btn.pin) + " VERY LONG PRESS").c_str());
                }
                else if (duration >= BUTTON_LONG_PRESS_MS) {
                    // 长按
                    btn.pendingEvent = ButtonEvent::LONG_PRESS;
                    btn.eventConsumed = false;
                    btn.waitingForDouble = false;
                    LOG_DEBUG("BUTTON", ("Pin " + String(btn.pin) + " LONG PRESS").c_str());
                }
                else {
                    // 短按：启动双击等待窗口
                    btn.pendingEvent = ButtonEvent::RELEASED;
                    btn.eventConsumed = false;

                    if (!btn.waitingForDouble) {
                        btn.waitingForDouble = true;
                        // 不立即发送 CLICK，等双击窗口过期再发
                    }
                }
            }
        }
    }

    btn.lastReading = btn.reading;

    // 3. 双击窗口过期处理
    if (btn.waitingForDouble &&
        (millis() - btn.releaseTime) >= BUTTON_DOUBLE_CLICK_MS) {
        btn.waitingForDouble = false;
        // 确认是单击（不是双击）
        btn.pendingEvent = ButtonEvent::CLICK;
        btn.eventConsumed = false;
        LOG_DEBUG("BUTTON", ("Pin " + String(btn.pin) + " CLICK").c_str());
    }

    // 4. 长按持续检测（按住不放，达到阈值时生成事件）
    if (btn.pressed && !btn.eventConsumed &&
        btn.pendingEvent == ButtonEvent::PRESSED) {
        unsigned long duration = millis() - btn.pressStart;

        if (duration >= BUTTON_VERY_LONG_PRESS_MS) {
            btn.pendingEvent = ButtonEvent::VERY_LONG_PRESS;
            btn.waitingForDouble = false;
            LOG_DEBUG("BUTTON", ("Pin " + String(btn.pin) + " VERY LONG PRESS (held)").c_str());
        }
        else if (duration >= BUTTON_LONG_PRESS_MS &&
                 btn.pendingEvent != ButtonEvent::LONG_PRESS) {
            btn.pendingEvent = ButtonEvent::LONG_PRESS;
            LOG_DEBUG("BUTTON", ("Pin " + String(btn.pin) + " LONG PRESS (held)").c_str());
        }
    }
}

void ButtonManager::_detectEvents(ButtonState& btn) {
    // 此方法已合入 _updateButton，保留空壳供扩展
}
