/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * 主程序入口
 * ============================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "Logger.h"
#include "SerialCommand.h"
#include "SensorManager.h"
#include "DisplayManager.h"
#include "ActuatorManager.h"
#include "ButtonManager.h"

// ============================================================
// 全局对象
// ============================================================

SerialCommand cmd;
ButtonManager buttons;
SensorManager sensor;
DisplayManager display;
RelayManager relay;
LedManager led;
BuzzerManager buzzer;
unsigned long bootTime;

// 系统状态
bool manualMode = false;   // 手动模式（暂停自动控制）
bool relayManualState = false;

// ============================================================
// 命令处理函数
// ============================================================

void cmdStatus(const char* args) {
    unsigned long uptime = (millis() - bootTime) / 1000;
    unsigned long h = uptime / 3600;
    unsigned long m = (uptime % 3600) / 60;
    unsigned long s = uptime % 60;
    SensorData data = sensor.getData();

    Serial.println();
    Serial.println(F("=== System Status ==="));
    Serial.print(F("  Firmware : ")); Serial.println(PROJECT_VERSION);
    Serial.print(F("  Uptime   : ")); Serial.print(h); Serial.print(F("h "));
    Serial.print(m); Serial.print(F("m ")); Serial.print(s); Serial.println(F("s"));

    Serial.print(F("  Sensor   : "));
    if (sensor.isOK()) Serial.println(F("OK"));
    else { Serial.print(F("ERROR - ")); Serial.println(sensor.getLastError()); }

    Serial.print(F("  Temp     : "));
    data.valid ? Serial.println(String(data.temperature, 1) + " C") : Serial.println(F("N/A"));
    Serial.print(F("  Humidity : "));
    data.valid ? Serial.println(String(data.humidity, 1) + " %") : Serial.println(F("N/A"));

    Serial.print(F("  Display  : "));
    Serial.println(display.isOK() ? F("OK") : F("Not found"));

    Serial.print(F("  Relay    : "));
    Serial.print(relay.isOn() ? F("ON") : F("OFF"));
    Serial.print(F(" (cycles: ")); Serial.print(relay.getCycleCount()); Serial.println(F(")"));
    Serial.print(F("  Mode     : "));
    Serial.println(manualMode ? F("MANUAL") : F("AUTO"));

    Serial.print(F("  Reads    : ")); Serial.print(sensor.getReadCount());
    Serial.print(F(" / ")); Serial.print(sensor.getErrorCount()); Serial.println(F(" errors"));
    Serial.print(F("  Free RAM : ")); Serial.print(ESP.getFreeHeap()); Serial.println(F(" bytes"));
    Serial.println();
}

void cmdRelay(const char* args) {
    if (strlen(args) == 0) {
        Serial.print(F("Relay is: "));
        Serial.println(relay.isOn() ? F("ON") : F("OFF"));
        return;
    }
    if (strcasecmp(args, "on") == 0) {
        manualMode = true;
        relay.turnOn();
        Serial.println(F("Relay ON (manual mode)"));
    } else if (strcasecmp(args, "off") == 0) {
        manualMode = true;
        relay.turnOff();
        Serial.println(F("Relay OFF (manual mode)"));
    } else if (strcasecmp(args, "auto") == 0) {
        manualMode = false;
        Serial.println(F("Resumed AUTO mode"));
    } else {
        Serial.println(F("Usage: RELAY on|off|auto"));
    }
}

void cmdSensor(const char* args) {
    SensorData data = sensor.getData();
    Serial.println(F("=== Sensor ==="));
    Serial.print(F("  Status  : "));
    sensor.isOK() ? Serial.println(F("OK")) : Serial.println(sensor.getLastError());
    Serial.print(F("  Temp    : ")); Serial.print(data.temperature, 1); Serial.println(F(" C"));
    Serial.print(F("  Humidity: ")); Serial.print(data.humidity, 1); Serial.println(F(" %"));
    Serial.print(F("  Valid   : ")); Serial.println(data.valid ? F("Yes") : F("No"));
    Serial.print(F("  Reads   : ")); Serial.print(sensor.getReadCount());
    Serial.print(F(" / ")); Serial.print(sensor.getErrorCount()); Serial.println(F(" errors"));
    Serial.println();
}

void cmdHelp(const char* args) { cmd.printHelp(); }

void cmdInfo(const char* args) {
    Serial.println(F("=== Device Info ==="));
    Serial.print(F("  Name    : ")); Serial.println(PROJECT_NAME);
    Serial.print(F("  Version : ")); Serial.println(PROJECT_VERSION);
    Serial.println(F("  MCU     : ESP32 DevKit V1"));
    Serial.print(F("  Flash   : ")); Serial.print(ESP.getFlashChipSize()/1024/1024); Serial.println(F(" MB"));
    Serial.print(F("  SDK     : ")); Serial.println(ESP.getSdkVersion());
    Serial.println();
}

void cmdReset(const char* args) {
    LOG_WARN("SYSTEM", "Software reset...");
    delay(1000);
    ESP.restart();
}

void cmdBuzzer(const char* args) {
    if (strlen(args) == 0) {
        Serial.println(F("Usage: BUZZER test|mute|unmute"));
        return;
    }
    if (strcasecmp(args, "test") == 0) {
        Serial.println(F("Testing buzzer..."));
        buzzer.beep();
    } else if (strcasecmp(args, "mute") == 0) {
        buzzer.mute();
    } else if (strcasecmp(args, "unmute") == 0) {
        buzzer.unmute();
    }
}

// ============================================================
// I2C 扫描
// ============================================================
void scanI2CBus() {
    LOG_INFO("I2C", "Scanning I2C bus...");
    byte error, address; int found = 0;
    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            Serial.print(F("[INFO ] [I2C    ] Device at 0x"));
            if (address < 16) Serial.print(F("0"));
            Serial.print(address, HEX);
            if (address == 0x38) Serial.println(F(" -> AHT20"));
            else if (address == 0x3C) Serial.println(F(" -> OLED"));
            else Serial.println(F(" -> Unknown"));
            found++;
        }
    }
    if (found == 0) LOG_WARN("I2C", "No devices found!");
    else { Serial.print(F("[INFO ] [I2C    ] ")); Serial.print(found); Serial.println(F(" device(s)")); }
}

// ============================================================
// 初始化
// ============================================================
void setup() {
    Serial.begin(SERIAL_BAUDRATE); delay(1000);
    Logger::printBanner();
    bootTime = millis();
    LOG_INFO("SYSTEM", "Booting...");

    // I2C
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_CLOCK_FREQ);
    LOG_INFO("I2C", "I2C bus OK");
    #if ENABLE_I2C_SCAN
    scanI2CBus();
    #endif

    // 硬件模块初始化
    sensor.begin();
    display.begin();
    relay.begin(RELAY_PIN, RELAY_ACTIVE_HIGH);
    led.begin(LED_RED_PIN, LED_GREEN_PIN, LED_BLUE_PIN);
    buzzer.begin(BUZZER_PIN);
    buttons.begin(BUTTON_A_PIN, BUTTON_B_PIN);

    // 启动后短促提示音
    buzzer.beep();

    // 初始 LED：白色（初始化完成）
    led.set(LedColor::COLOR_WHITE, LedPattern::SLOW_BLINK);

    // 命令注册
    cmd.begin();
    cmd.addCommand("STATUS", "System status", cmdStatus);
    cmd.addCommand("SENSOR", "Sensor diagnostics", cmdSensor);
    cmd.addCommand("RELAY",  "Relay on|off|auto", cmdRelay);
    cmd.addCommand("BUZZER", "Buzzer test|mute|unmute", cmdBuzzer);
    cmd.addCommand("HELP",   "Available commands", cmdHelp);
    cmd.addCommand("INFO",   "Device information", cmdInfo);
    cmd.addCommand("RESET",  "Restart device", cmdReset);

    LOG_INFO("SYSTEM", "Ready.");
    Logger::printSeparator();
    Serial.print(F("> "));
}

// ============================================================
// 主循环
// ============================================================
void loop() {
    cmd.update();
    sensor.readSensor();
    led.update();
    buzzer.update();
    buttons.update();

    SensorData data = sensor.getData();

    // 自动控制逻辑（后续 Task 会独立成 HumidifierController）
    if (!manualMode && data.valid) {
        if (data.humidity < HUMIDITY_LOW_THRESHOLD) {
            relay.turnOn();
        } else if (data.humidity > HUMIDITY_HIGH_THRESHOLD) {
            relay.turnOff();
        }
        // 在滞回区内保持当前状态
    }

    // LED 状态
    if (!sensor.isOK()) {
        led.set(LedColor::RED, LedPattern::FAST_BLINK);
        buzzer.alarmSensorError();
    } else if (relay.isOn()) {
        led.set(LedColor::BLUE, LedPattern::SOLID);
        buzzer.stop();
    } else {
        led.set(LedColor::GREEN, LedPattern::SOLID);
        buzzer.stop();
    }

    // ---- 按钮事件处理 ----
    ButtonEvent evtA = buttons.getEventA();
    ButtonEvent evtB = buttons.getEventB();

    // 按钮 A：模式切换
    if (evtA == ButtonEvent::CLICK) {
        display.nextPage();  // 切换 OLED 页面
    } else if (evtA == ButtonEvent::DOUBLE_CLICK) {
        // 双击：手动开关继电器
        manualMode = true;
        if (relay.isOn()) relay.turnOff();
        else relay.turnOn();
        LOG_INFO("MAIN", "Relay toggled (manual)");
    } else if (evtA == ButtonEvent::LONG_PRESS) {
        // 长按：切换手动/自动模式
        manualMode = !manualMode;
        LOG_INFO("MAIN", manualMode ? "Switched to MANUAL mode" : "Switched to AUTO mode");
    }

    // 按钮 B：系统控制
    if (evtB == ButtonEvent::CLICK) {
        // 单击：恢复默认阈值
        LOG_INFO("MAIN", "Reset to default thresholds");
    } else if (evtB == ButtonEvent::LONG_PRESS) {
        // 长按：切换静音
        if (buzzer.isMuted()) buzzer.unmute();
        else buzzer.mute();
    } else if (evtB == ButtonEvent::VERY_LONG_PRESS) {
        // 超长按5秒：恢复出厂设置
        LOG_WARN("MAIN", "Factory reset requested...");
        ESP.restart();
    }

    // 显示
    const char* stateText;
    if (!sensor.isOK()) stateText = "ERROR";
    else if (manualMode) stateText = "MANUAL";
    else if (relay.isOn()) stateText = "HEATING";
    else stateText = "STANDBY";
    display.update(data, relay.isOn(), stateText);

    // 心跳
    static unsigned long lastHb = 0;
    if (millis() - lastHb >= 30000) {
        LOG_DEBUG("MAIN", ("Heartbeat " + String((millis()-bootTime)/1000) + "s").c_str());
        lastHb = millis();
    }
}
