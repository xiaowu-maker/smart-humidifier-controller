---
name: shc-firmware-architecture
description: SHC 固件架构设计详解（面试重点）
metadata:
  type: project-reference
  project: SHC-项目主页
  audience: [面试官, 技术评审, 学习参考]
tags:
  - 架构设计
  - 嵌入式
  - C++
  - 设计模式
  - 面试
---

# 🏗️ SHC 固件架构详解

> 本文档面向面试官/技术评审，详细说明架构设计思路和技术决策。

---

## 一、架构概览：四层分层设计

```
┌──────────────────────────────────────────────────┐
│                应用层 (Application)                │
│  main.cpp: 主循环 + 滞回控制算法 + 按钮交互         │
│  总管所有模块的协调与决策                           │
├──────────────────────────────────────────────────┤
│              硬件抽象层 (HAL)                       │
│  SensorManager   DisplayManager   ActuatorManager  │
│  (AHT20传感器)   (SSD1306 OLED)  (继电器+LED+蜂鸣器)│
│  每类硬件一个类，对外暴露统一接口                    │
├──────────────────────────────────────────────────┤
│               工具层 (Utilities)                    │
│  Logger     SerialCommand    ButtonManager         │
│  (日志)     (命令解析)       (按钮检测)             │
│  不直接操作硬件，提供通用服务                       │
├──────────────────────────────────────────────────┤
│              驱动层 (Drivers)                      │
│  I2C (Wire.h)  GPIO (digitalWrite)  UART (Serial) │
│  ESP32 Arduino Core + 芯片底层驱动                 │
└──────────────────────────────────────────────────┘
```

### 为什么选择四层架构？

| 层 | 职责 | 好处 |
|:--|:--|:--|
| 驱动层 | 直接操作硬件寄存器 | 换芯片时只改这层 |
| HAL | 封装硬件为对象 | 上层不需要知道 I2C 地址、引脚号 |
| 工具层 | 通用服务 | 可复用到其他项目 |
| 应用层 | 业务逻辑 | 只关心"什么时候开""什么时候关" |

**核心原则**：**上层不依赖下层实现细节**。比如 `main.cpp` 不知道 AHT20 的 I2C 地址是多少，它只调用 `sensor.readSensor()`。

---

## 二、模块职责矩阵

| 模块 | 头文件 | 实现 | 行数估 | 依赖硬件 | 核心职责 |
|:--|:--|:--|:--|:--|:--|
| Logger | `Logger.h` | `Logger.cpp` | ~60 | UART | 分级日志输出 |
| SerialCommand | `SerialCommand.h` | `SerialCommand.cpp` | ~120 | UART | 命令注册/解析/执行 |
| SensorManager | `SensorManager.h` | `SensorManager.cpp` | ~160 | AHT20 (I2C 0x38) | 传感器数据读取与验证 |
| DisplayManager | `DisplayManager.h` | `DisplayManager.cpp` | ~200 | SSD1306 (I2C 0x3C) | OLED 3页面显示 |
| ActuatorManager | `ActuatorManager.h` | `ActuatorManager.cpp` | ~300 | GPIO×5 | 继电器+LED+蜂鸣器三合一 |
| ButtonManager | `ButtonManager.h` | `ButtonManager.cpp` | ~180 | GPIO×2 | 双按钮事件检测 |
| main.cpp | - | `main.cpp` | ~290 | 全部 | 主循环+控制逻辑 |

---

## 三、关键设计决策

### 3.1 为什么用非阻塞编程（No delay()）？

```cpp
// ❌ 阻塞写法（初学者常见错误）
void loop() {
    readSensor();
    delay(2000);  // CPU 空转 2 秒！这段时间什么都做不了
    updateDisplay();
    delay(1000);  // 又空转 1 秒！
}

// ✅ 非阻塞写法（本项目实际使用）
void loop() {
    cmd.update();       // 检查串口输入
    sensor.readSensor(); // 内部用 millis() 判断是否到了采样时间
    led.update();        // 非阻塞闪烁
    buzzer.update();     // 非阻塞报警
    buttons.update();    // 非阻塞消抖
    // 全部在一个循环里，互不阻塞！
}
```

**为什么重要**：
- delay() 期间 CPU 完全空转，无法处理任何其他事情
- 如果用户在 delay() 期间按下按钮，事件会丢失
- 如果串口数据在 delay() 期间到达，可能被丢弃
- 非阻塞让一个单核 CPU "看起来"在同时做多件事

### 3.2 滞回控制算法（核心算法）

```
         湿度
          ↑
     55%  ├──────── 关闭加湿器（上限阈值）
          │
          │   ☐ 滞回区间（死区）
          │   保持当前状态不变
          │   避免频繁开关
          │
     45%  ├──────── 开启加湿器（下限阈值）
          │
          └──────────────────→ 时间
```

**为什么不用单一阈值（如 50%）？**

假设阈值设为 50%：
```
时间 → 湿度 49.9% → 开 →
      加湿 → 湿度 50.1% → 关 →
      自然蒸发 → 湿度 49.9% → 开 →
      加湿 → 湿度 50.1% → 关 →
      ... 每分钟反复开关数十次！
```

使用 45%/55% 滞回区间：
```
湿度 44% → 开（加湿中...）
湿度 56% → 关（自然蒸发...）
湿度 53% → 不动作（在 45%~55% 滞回区间内，保持关闭）
湿度 46% → 不动作（还在滞回区间内，保持关闭）
湿度 44% → 开（跌破下限，重新开启）
```

**实际代码**（`main.cpp:226-233`）：
```cpp
if (!manualMode && data.valid) {
    if (data.humidity < HUMIDITY_LOW_THRESHOLD) {    // < 45%
        relay.turnOn();
    } else if (data.humidity > HUMIDITY_HIGH_THRESHOLD) { // > 55%
        relay.turnOff();
    }
    // 45%~55% 之间：什么都不做，保持当前状态
}
```

### 3.3 三重保护机制

| 保护层 | 机制 | 作用 |
|:--|:--|:--|
| 第1层 | **滞回区间**（45%~55%） | 防止临界点抖动 |
| 第2层 | **最短运行时间**（60秒） | 防止短时间反复开关损坏继电器 |
| 第3层 | **最短关闭时间**（30秒） | 防止加湿器压缩机频繁启停 |

```cpp
// RelayManager 中的最短时间保护
void RelayManager::turnOn() {
    if (!_state && timeSinceLastChange() < RELAY_MIN_OFF_TIME_MS) {
        LOG_WARN("RELAY", "Min off time not met, refusing to turn ON");
        return;  // 拒绝操作！
    }
    // ... 正常开启
}
```

### 3.4 优雅降级模式（Fail-Safe）

```
传感器正常       → 正常控制模式（自动调节湿度）
传感器短暂异常   → 使用上次缓存数据 + 记录错误计数
传感器连续5次异常 → 告警模式（蜂鸣器响 + LED红色快闪 + 继电器强制关闭）
传感器恢复       → 自动切回正常控制模式
```

**核心思想**：硬件故障不能让系统崩溃。能降级运行就降级运行，能告警就告警，绝对不能失控。

```cpp
// main.cpp 中的 LED 状态映射
if (!sensor.isOK()) {
    led.set(LedColor::RED, LedPattern::FAST_BLINK);  // 传感器故障
    buzzer.alarmSensorError();                        // 蜂鸣器告警
} else if (relay.isOn()) {
    led.set(LedColor::BLUE, LedPattern::SOLID);       // 加湿中
    buzzer.stop();
} else {
    led.set(LedColor::GREEN, LedPattern::SOLID);      // 正常待机
    buzzer.stop();
}
```

### 3.5 命令注册模式

传统做法（if-else 大法）：
```cpp
// ❌ 每加一个命令就要改主循环
if (cmd == "STATUS") { ... }
else if (cmd == "SENSOR") { ... }
else if (cmd == "HELP") { ... }
// 100 个命令 = 100 个 if-else
```

本项目做法（注册模式）：
```cpp
// ✅ 注册命令，主循环不需要修改
cmd.addCommand("STATUS", "System status", cmdStatus);
cmd.addCommand("SENSOR", "Sensor diagnostics", cmdSensor);
cmd.addCommand("RELAY",  "Relay on|off|auto", cmdRelay);
cmd.addCommand("BUZZER", "Buzzer test|mute|unmute", cmdBuzzer);
cmd.addCommand("HELP",   "Available commands", cmdHelp);
cmd.addCommand("INFO",   "Device information", cmdInfo);
cmd.addCommand("RESET",  "Restart device", cmdReset);
```

**为什么更好**：
- 添加新命令不需要修改任何现有代码（开闭原则）
- 命令和处理器解耦
- `HELP` 命令可以自动生成命令列表

---

## 四、数据流图

```
        ┌─────────────┐
        │  AHT20 传感器 │  I2C 0x38
        └──────┬──────┘
               │ 原始温湿度数据
               ▼
        ┌─────────────┐
        │SensorManager │  数据验证（范围+跳变）
        │  getData()   │  → SensorData {temp, humidity, valid}
        └──────┬──────┘
               │
    ┌──────────┼──────────┐
    │          │          │
    ▼          ▼          ▼
┌───────┐ ┌───────┐ ┌───────────┐
│Display│ │Control│ │SerialCmd │
│Manager│ │ Logic │ │(STATUS)  │
│OLED   │ │滞回判断│ │串口输出   │
└───────┘ └───┬───┘ └───────────┘
              │
    ┌─────────┼─────────┐
    │         │         │
    ▼         ▼         ▼
┌──────┐ ┌──────┐ ┌────────┐
│Relay │ │ LED  │ │Buzzer  │
│开关  │ │颜色  │ │报警    │
└──────┘ └──────┘ └────────┘
```

---

## 五、状态机设计

系统有三种核心状态：

```
         ┌──────────┐
    ┌───→│  STANDBY  │←───┐
    │    │ 湿度正常   │    │
    │    │ LED=绿色   │    │
    │    └─────┬──────┘    │
    │          │           │
    │   湿度<45%│    湿度>55%│
    │          │           │
    │    ┌─────▼──────┐    │
    │    │  HEATING   │    │
    │    │ 加湿器运行  │    │
    │    │ LED=蓝色   │    │
    │    └─────┬──────┘    │
    │          │           │
    │          └───────────┘
    │      45%<湿度<55%（滞回区，不切换）
    │
    └── 传感器故障 ──→ ┌──────────┐
                       │  ERROR    │
                       │ 传感器异常 │
                       │ LED=红色  │
                       │ Buzzer响  │
                       └──────────┘
```

---

## 六、内存与性能

```
编译结果 (2026-07-11)
─────────────────────────────
RAM      :  22KB / 320KB  (7.0%)
Flash    : 280KB / 1.3MB  (21.5%)
主循环频率: ~500Hz（每个循环 <2ms）
传感器采样: 每 2 秒
显示刷新  : 每 1 秒
```

**内存分配说明**：
- 全局对象（静态分配）：~2KB（7个模块的类实例）
- Arduino 框架开销：~10KB
- 可用堆空间：~300KB（足够后续 V2.0 WiFi + Web Server）

---

## 📂 相关笔记

- [[SHC-项目主页]] — 返回项目总览
- [[SHC-开发日志]] — 每日开发记录
- [[SHC-代码模块索引]] — 每个代码文件详解
- [[SHC-编译与Bug记录]] — 编译记录和 Bug 分析
- [[SHC-面试准备手册]] — 面试话术与要点
