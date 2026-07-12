/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * ActuatorManager - 执行器管理模块（实现文件）
 * ============================================================
 */

#include "ActuatorManager.h"
#include "config.h"
#include "Logger.h"

// ============================================================
// RelayManager
// ============================================================

void RelayManager::begin(uint8_t pin, bool activeHigh) {
    _pin = pin;
    _activeHigh = activeHigh;
    _state = false;
    _lastChangeTime = 0;
    _cycleCount = 0;

    pinMode(_pin, OUTPUT);
    _applyState();  // 默认关闭

    LOG_INFO("RELAY", ("Pin GPIO" + String(_pin) +
              (_activeHigh ? " (Active HIGH)" : " (Active LOW)")).c_str());
}

void RelayManager::turnOn() {
    // 检查最短关闭时间保护
    if (!_state && timeSinceLastChange() < RELAY_MIN_OFF_TIME_MS) {
        LOG_WARN("RELAY", "Min off time not met, refusing to turn ON");
        return;
    }

    if (!_state) {
        _state = true;
        _lastChangeTime = millis();
        _cycleCount++;
        _applyState();
        LOG_INFO("RELAY", "Relay ON");
    }
}

void RelayManager::turnOff() {
    // 检查最短开启时间保护
    if (_state && timeSinceLastChange() < RELAY_MIN_ON_TIME_MS) {
        LOG_WARN("RELAY", "Min on time not met, refusing to turn OFF");
        return;
    }

    if (_state) {
        _state = false;
        _lastChangeTime = millis();
        _applyState();
        LOG_INFO("RELAY", "Relay OFF");
    }
}

void RelayManager::toggle() {
    if (_state) turnOff(); else turnOn();
}

bool RelayManager::isOn() {
    return _state;
}

unsigned long RelayManager::timeSinceLastChange() {
    return millis() - _lastChangeTime;
}

unsigned long RelayManager::getCycleCount() {
    return _cycleCount;
}

void RelayManager::_applyState() {
    if (_activeHigh) {
        digitalWrite(_pin, _state ? HIGH : LOW);
    } else {
        digitalWrite(_pin, _state ? LOW : HIGH);
    }
}

// ============================================================
// LedManager
// ============================================================

void LedManager::begin(uint8_t pinR, uint8_t pinG, uint8_t pinB) {
    _pinR = pinR;
    _pinG = pinG;
    _pinB = pinB;
    _color = LedColor::OFF;
    _pattern = LedPattern::SOLID;
    _visible = true;
    _lastToggleTime = 0;
    _blinkStep = 0;

    pinMode(_pinR, OUTPUT);
    pinMode(_pinG, OUTPUT);
    pinMode(_pinB, OUTPUT);
    turnOff();

    LOG_INFO("LED", "RGB LED ready (R=GPIO16, G=GPIO17, B=GPIO18)");
}

void LedManager::set(LedColor color, LedPattern pattern) {
    _color = color;
    _pattern = pattern;
    _visible = true;
    _lastToggleTime = millis();
    _blinkStep = 0;

    if (pattern == LedPattern::SOLID) {
        // 常亮模式直接设置颜色
        LedColor saved = _color;
        _color = color;
        switch (color) {
            case LedColor::GREEN:  _writeRGB(0, 1, 0); break;
            case LedColor::BLUE:   _writeRGB(0, 0, 1); break;
            case LedColor::RED:    _writeRGB(1, 0, 0); break;
            case LedColor::YELLOW: _writeRGB(1, 1, 0); break;
            case LedColor::COLOR_WHITE:  _writeRGB(1, 1, 1); break;
            case LedColor::OFF:
            default:              _writeRGB(0, 0, 0); break;
        }
        _color = saved;
    }
}

void LedManager::turnOff() {
    _writeRGB(0, 0, 0);
    _color = LedColor::OFF;
    _pattern = LedPattern::SOLID;
}

void LedManager::update() {
    // 非闪烁模式不需要处理
    if (_pattern == LedPattern::SOLID) return;

    unsigned long now = millis();
    int interval;

    // 根据模式决定切换间隔
    switch (_pattern) {
        case LedPattern::SLOW_BLINK:   interval = 500;  break;
        case LedPattern::FAST_BLINK:   interval = 150;  break;
        case LedPattern::DOUBLE_BLINK: interval = 100;  break;
        default: return;
    }

    if (now - _lastToggleTime >= (unsigned long)interval) {
        _lastToggleTime = now;

        if (_pattern == LedPattern::DOUBLE_BLINK) {
            // 双闪：亮-灭-亮-灭-长灭
            _blinkStep = (_blinkStep + 1) % 6;
            _visible = (_blinkStep < 2 || _blinkStep == 4);
        } else {
            _visible = !_visible;
        }

        if (_visible) {
            switch (_color) {
                case LedColor::GREEN:  _writeRGB(0, 1, 0); break;
                case LedColor::BLUE:   _writeRGB(0, 0, 1); break;
                case LedColor::RED:    _writeRGB(1, 0, 0); break;
                case LedColor::YELLOW: _writeRGB(1, 1, 0); break;
                case LedColor::COLOR_WHITE:  _writeRGB(1, 1, 1); break;
                default:              _writeRGB(0, 0, 0); break;
            }
        } else {
            _writeRGB(0, 0, 0);
        }
    }
}

LedColor LedManager::getColor() {
    return _color;
}

void LedManager::_writeRGB(bool r, bool g, bool b) {
    digitalWrite(_pinR, r ? HIGH : LOW);
    digitalWrite(_pinG, g ? HIGH : LOW);
    digitalWrite(_pinB, b ? HIGH : LOW);
}

// ============================================================
// BuzzerManager
// ============================================================

void BuzzerManager::begin(uint8_t pin) {
    _pin = pin;
    _active = false;
    _muted = false;
    _pattern = 0;
    _step = 0;
    _lastToggleTime = 0;
    _buzzerState = false;

    pinMode(_pin, OUTPUT);
    _setPin(false);

    LOG_INFO("BUZZER", "Buzzer ready (Pin GPIO19)");
}

void BuzzerManager::beep() {
    if (_muted) return;
    _setPin(true);
    delay(200);
    _setPin(false);
}

void BuzzerManager::doubleBeep() {
    if (_muted) return;
    beep();
    delay(150);
    beep();
}

void BuzzerManager::alarmSensorError() {
    if (_muted) return;
    _active = true;
    _pattern = 1;
    _step = 0;
}

void BuzzerManager::alarmWifiLost() {
    if (_muted) return;
    _active = true;
    _pattern = 2;
    _step = 0;
}

void BuzzerManager::alarmCritical() {
    if (_muted) return;
    _active = true;
    _pattern = 3;
    _step = 0;
}

void BuzzerManager::stop() {
    _active = false;
    _pattern = 0;
    _setPin(false);
}

void BuzzerManager::mute() {
    _muted = true;
    stop();
    LOG_INFO("BUZZER", "Muted");
}

void BuzzerManager::unmute() {
    _muted = false;
    LOG_INFO("BUZZER", "Unmuted");
}

bool BuzzerManager::isMuted() {
    return _muted;
}

void BuzzerManager::update() {
    if (!_active || _muted || _pattern == 0) return;

    unsigned long now = millis();

    switch (_pattern) {
        case 1: {  // 传感器故障：响 500ms，停 1000ms
            int intervals[] = {500, 1000};
            if (now - _lastToggleTime >= (unsigned long)intervals[_step % 2]) {
                _lastToggleTime = now;
                _buzzerState = !_buzzerState;
                _setPin(_buzzerState);
                _step++;
            }
            break;
        }
        case 2: {  // WiFi 断开：三短一长
            int intervals[] = {100, 100, 100, 100, 100, 100, 500, 2000};
            if (now - _lastToggleTime >= (unsigned long)intervals[_step]) {
                _lastToggleTime = now;
                _buzzerState = !_buzzerState;
                _setPin(_buzzerState);
                _step = (_step + 1) % 8;
            }
            break;
        }
        case 3: {  // 严重故障：急促鸣叫
            if (now - _lastToggleTime >= 100) {
                _lastToggleTime = now;
                _buzzerState = !_buzzerState;
                _setPin(_buzzerState);
            }
            break;
        }
    }
}

void BuzzerManager::_setPin(bool on) {
    digitalWrite(_pin, on ? HIGH : LOW);
}
