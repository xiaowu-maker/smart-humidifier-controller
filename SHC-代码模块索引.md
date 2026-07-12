---
name: shc-code-module-index
description: SHC 所有代码模块的索引与功能说明
metadata:
  type: project-reference
  project: SHC-项目主页
tags:
  - 代码
  - 索引
  - 模块说明
---

# 📦 SHC 代码模块索引

> 每个文件的职责、核心功能、对外接口一览。
> 源码路径：`SmartHumidifier/firmware/`

---

## 📁 工程配置

### `platformio.ini`

```
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
board_build.flash_mode = dio
board_build.f_cpu = 240000000L
```

| 配置项 | 值 | 原因 |
|:--|:--|:--|
| platform | espressif32 | 乐鑫 ESP32 平台 |
| board | esp32dev | 最常见的 ESP32 DevKit V1 |
| framework | arduino | Arduino 框架（非 ESP-IDF） |
| monitor_speed | 115200 | ESP32 标准串口速率 |
| flash_mode | dio | 双线 IO，读写速度快 |
| cpu_freq | 240MHz | ESP32 最大主频 |

**依赖库**（PlatformIO 自动管理版本）：
- Adafruit AHTX0 @ ^2.0.5 — AHT20 传感器驱动
- Adafruit SSD1306 @ ^2.5.7 — OLED 屏幕驱动
- Adafruit GFX @ ^1.11.9 — 图形底层（画线/文字/形状）
- Adafruit BusIO @ ^1.14.5 — I2C/SPI 总线抽象
- ArduinoJson @ ^7.0.4 — JSON 序列化（为 V2.0 API 预留）

---

## 📁 配置头文件

### `include/config.h`

**全局配置中心**，所有可调参数集中在这里，修改阈值不需要翻代码。

#### 主要配置段

```cpp
// ===== 项目信息 =====
#define PROJECT_NAME "SmartHumidifier"
#define PROJECT_VERSION "1.0.0"

// ===== 引脚映射 =====
#define I2C_SDA_PIN     21     // AHT20 + OLED 共享 SDA
#define I2C_SCL_PIN     22     // AHT20 + OLED 共享 SCL
#define RELAY_PIN       4      // 继电器控制
#define LED_RED_PIN     16     // RGB LED 红
#define LED_GREEN_PIN   17     // RGB LED 绿
#define LED_BLUE_PIN    18     // RGB LED 蓝
#define BUZZER_PIN      19     // 蜂鸣器
#define BUTTON_A_PIN    13     // 按钮A（模式切换）
#define BUTTON_B_PIN    14     // 按钮B（系统控制）

// ===== 控制阈值 =====
#define HUMIDITY_LOW_THRESHOLD   45.0f   // 低于此值→开启加湿器
#define HUMIDITY_HIGH_THRESHOLD  55.0f   // 高于此值→关闭加湿器

// ===== 时间常量 =====
#define SENSOR_READ_INTERVAL_MS    2000   // 每2秒读一次传感器
#define DISPLAY_UPDATE_INTERVAL_MS 1000   // 每1秒刷新屏幕
#define RELAY_MIN_ON_TIME_MS       60000  // 继电器最短开启60秒
#define RELAY_MIN_OFF_TIME_MS      30000  // 继电器最短关闭30秒
#define BUTTON_DEBOUNCE_MS         30     // 按钮消抖30ms
#define BUTTON_LONG_PRESS_MS       1000   // 长按1秒
#define BUTTON_DOUBLE_CLICK_MS     400    // 双击窗口400ms
#define BUTTON_VERY_LONG_PRESS_MS  3000   // 超长按3秒

// ===== 日志级别 =====
#define LOG_LEVEL LOG_LEVEL_DEBUG  // 开发阶段全开
```

---

## 📁 src/ — 核心模块

### 1. `Logger.h` / `Logger.cpp` — 日志系统

| 项目 | 详情 |
|:--|:--|
| 依赖 | `config.h` |
| 依赖硬件 | UART (Serial) |
| 代码行数 | ~60 行 |

**4 级日志**：

```cpp
#define LOG_DEBUG(tag, msg)  // 调试信息（开发用，发布可关闭）
#define LOG_INFO(tag, msg)   // 一般信息（正常运行记录）
#define LOG_WARN(tag, msg)   // 警告（不影响运行但需要注意）
#define LOG_ERROR(tag, msg)  // 错误（需要立即处理）
```

**输出格式**：`[LEVEL] [TAG    ] message`

```
[INFO ] [SYSTEM ] Booting...
[INFO ] [I2C    ] I2C bus OK
[INFO ] [RELAY  ] Relay ON
[WARN ] [SENSOR ] Reading failed: I2C timeout
```

**设计要点**：
- 宏封装：编译期可通过 `LOG_LEVEL` 关闭低级日志，节省 Flash
- TAG 固定 7 字符宽度：输出对齐，肉眼友好
- `printBanner()` 输出启动横幅

---

### 2. `SerialCommand.h` / `SerialCommand.cpp` — 串口命令

| 项目 | 详情 |
|:--|:--|
| 依赖 | `Logger.h` |
| 依赖硬件 | UART (Serial) |
| 代码行数 | ~120 行 |
| 最大命令数 | 16 个 |

**命令注册模式**：

```cpp
// 注册（在 setup() 中）
cmd.addCommand("STATUS", "System status", cmdStatus);
cmd.addCommand("SENSOR", "Sensor diagnostics", cmdSensor);
cmd.addCommand("RELAY",  "Relay on|off|auto", cmdRelay);

// 主循环
void loop() {
    cmd.update();  // 非阻塞，检查串口缓冲区
}
```

**V1.0 已注册的 7 个命令**：

| 命令 | 用法 | 功能 |
|:--|:--|:--|
| `STATUS` | `STATUS` | 完整系统状态（传感器/继电器/运行时间/内存） |
| `SENSOR` | `SENSOR` | 传感器详细诊断 |
| `RELAY` | `RELAY on\|off\|auto` | 手动控制继电器 / 恢复自动模式 |
| `BUZZER` | `BUZZER test\|mute\|unmute` | 测试蜂鸣器 / 静音控制 |
| `HELP` | `HELP` | 列出所有可用命令 |
| `INFO` | `INFO` | 设备信息（名称/版本/芯片/Flash） |
| `RESET` | `RESET` | 软件重启 |

**设计要点**：
- 大小写不敏感：`status`、`STATUS`、`Status` 都能识别
- `update()` 非阻塞：每次只读一个字节，缓冲区满了再处理
- 自动 trim 前后空格和换行符

---

### 3. `SensorManager.h` / `SensorManager.cpp` — AHT20 传感器

| 项目 | 详情 |
|:--|:--|
| 依赖 | Adafruit_AHTX0, Adafruit_BusIO, Wire |
| 依赖硬件 | AHT20 (I2C 地址 0x38) |
| 代码行数 | ~160 行 |

**数据结构**：

```cpp
struct SensorData {
    float temperature;   // 温度（°C）
    float humidity;      // 湿度（%RH）
    bool valid;          // 数据是否有效
};
```

**核心 API**：

```cpp
void begin();                          // 初始化传感器
void readSensor();                     // 非阻塞读取（内部检查时间间隔）
SensorData getData();                  // 获取最新数据
bool isOK();                           // 传感器是否正常
const char* getLastError();           // 最后一次错误信息
int getReadCount();                   // 总读取次数
int getErrorCount();                  // 总错误次数
void setTemperatureOffset(float);     // 温度校准偏移
void setHumidityOffset(float);        // 湿度校准偏移
```

**数据验证规则**：

| 检查项 | 范围 | 失败处理 |
|:--|:--|:--|
| 温度范围 | -40°C ~ 85°C | 丢弃，使用缓存值 |
| 湿度范围 | 0% ~ 100% | 丢弃，使用缓存值 |
| 温度跳变 | Δ > 10°C（两次采样间） | 丢弃，标记可疑 |
| 湿度跳变 | Δ > 20%（两次采样间） | 丢弃，标记可疑 |
| 连续错误 | > 5 次 | 标记传感器故障 |

**设计要点**：
- `readSensor()` 内部用 `millis()` 计时，每 2 秒才真正读取一次
- 读取失败 → 计数器累加 → 达到阈值 → `isOK()` 返回 false
- 优雅降级：`getData()` 在传感器故障时返回最后一次有效数据 + `valid=false`

---

### 4. `DisplayManager.h` / `DisplayManager.cpp` — OLED 显示

| 项目 | 详情 |
|:--|:--|
| 依赖 | Adafruit_SSD1306, Adafruit_GFX, Wire |
| 依赖硬件 | SSD1306 128×64 OLED (I2C 地址 0x3C) |
| 代码行数 | ~200 行 |

**3 页面设计**：

| 页面 | 内容 | 用途 |
|:--|:--|:--|
| 第 0 页（主页） | 温度 + 湿度 + 系统状态 | 日常使用 |
| 第 1 页（网络） | 占位（V2.0 填充） | WiFi 信息 |
| 第 2 页（系统） | 版本号 + 运行时间 | 调试用 |

**页面切换**：按钮 A 短按 → `display.nextPage()`

**UI 元素**：
- 反白标题栏（黑色背景 + 白色文字）
- 温度用 `o` + `C` 模拟 `°C`（Adafruit GFX 默认字体不支持 ° 符号）
- 页面指示器：`●○○` / `○●○` / `○○●`
- 状态文字：`STANDBY` / `HEATING` / `MANUAL` / `ERROR`

**核心 API**：

```cpp
void begin();                        // 初始化 OLED
void update(SensorData, bool relay, const char* state);  // 刷新画面
void nextPage();                     // 切换到下一页
bool isOK();                         // OLED 是否正常
```

---

### 5. `ActuatorManager.h` / `ActuatorManager.cpp` — 执行器三合一

| 项目 | 详情 |
|:--|:--|
| 依赖 | `config.h`, `Logger.h` |
| 依赖硬件 | GPIO×5（继电器 + RGB LED + 蜂鸣器） |
| 代码行数 | ~300 行 |

**包含三个子管理器**：

#### 5a. RelayManager（继电器）

```cpp
void begin(uint8_t pin, bool activeHigh);
void turnOn();                      // 开（含最短关闭时间保护）
void turnOff();                     // 关（含最短开启时间保护）
void toggle();                      // 切换
bool isOn();                        // 当前状态
unsigned long getCycleCount();     // 开合次数统计
```

**保护机制**：
- 开启后至少保持 60 秒才能关闭（`MIN_ON_TIME`）
- 关闭后至少保持 30 秒才能开启（`MIN_OFF_TIME`）
- 支持 activeHigh / activeLow 两种继电器模块

#### 5b. LedManager（RGB LED）

```cpp
void begin(uint8_t pinR, uint8_t pinG, uint8_t pinB);
void set(LedColor, LedPattern);    // 设置颜色 + 闪烁模式
void turnOff();
void update();                      // 非阻塞闪烁更新
```

**颜色枚举**：

| 颜色 | 含义 |
|:--|:--|
| `GREEN` | 正常待机 |
| `BLUE` | 正在加湿 |
| `RED` | 传感器/系统异常 |
| `YELLOW` | 警告（预留 V2.0 WiFi 断开） |
| `COLOR_WHITE` | 初始化中 |

> ⚠️ 用 `COLOR_WHITE` 而非 `WHITE`，因为 SSD1306 库用 `#define WHITE` 宏，会污染枚举命名空间。参见 [[SHC-编译与Bug记录#Bug 4]]

**闪烁模式**：

| 模式 | 周期 | 用途 |
|:--|:--|:--|
| `SOLID` | 常亮 | 正常状态 |
| `SLOW_BLINK` | 1 秒周期 | 初始化中 |
| `FAST_BLINK` | 0.3 秒周期 | 传感器故障 |
| `DOUBLE_BLINK` | 双闪 | 预留 |

#### 5c. BuzzerManager（蜂鸣器）

```cpp
void begin(uint8_t pin);
void beep();                        // 短促一声
void doubleBeep();                  // 短促两声
void alarmSensorError();            // 模式1：500ms/1000ms 间歇
void alarmWifiLost();               // 模式2：三短一长
void alarmCritical();               // 模式3：连续急促
void stop();
void mute();                        // 静音
void unmute();
void update();                      // 非阻塞报警更新
```

---

### 6. `ButtonManager.h` / `ButtonManager.cpp` — 按钮检测

| 项目 | 详情 |
|:--|:--|
| 依赖 | `config.h`, `Logger.h` |
| 依赖硬件 | GPIO×2（内部上拉） |
| 代码行数 | ~180 行 |

**事件类型**：

| 事件 | 触发条件 | 用途示例 |
|:--|:--|:--|
| `NONE` | 无事件 | - |
| `PRESSED` | 刚按下 | 可忽略 |
| `RELEASED` | 刚松开 | 可忽略 |
| `CLICK` | 按下 < 1 秒松开 | 切换 OLED 页面 |
| `DOUBLE_CLICK` | 两次单击间隔 < 400ms | 手动开关继电器 |
| `LONG_PRESS` | 按下 > 1 秒 | 切换手动/自动模式 |
| `VERY_LONG_PRESS` | 按下 > 3 秒 | 恢复出厂设置 |

**消抖策略**：
1. 电平变化后锁存 30ms（`BUTTON_DEBOUNCE_MS`）
2. 30ms 内电平不变 → 确认状态变化
3. 松开后启动 400ms 双击等待窗口
4. 400ms 内再次按下 → 双击；超时 → 单击

**按钮分配**：

| 按钮 | 引脚 | 单击 | 双击 | 长按 1s | 超长按 5s |
|:--|:--|:--|:--|:--|:--|
| A | GPIO13 | 切换 OLED 页面 | 手动开关继电器 | 切换自动/手动 | - |
| B | GPIO14 | 恢复默认阈值 | - | 切换静音 | 恢复出厂设置 |

**事件查询模式**（读取后自动清除）：
```cpp
ButtonEvent evt = buttons.getEventA();
if (evt == ButtonEvent::CLICK) {
    // 处理单击
}
// 下次调用 getEventA() 返回 NONE（事件已消耗）
```

---

### 7. `main.cpp` — 主程序

| 项目 | 详情 |
|:--|:--|
| 依赖 | 所有 6 个模块 |
| 代码行数 | ~290 行 |

**setup() 执行顺序**：
1. 初始化串口（115200）
2. 输出启动横幅
3. 初始化 I2C 总线
4. I2C 扫描（打印找到的设备）
5. 依次初始化 6 个硬件模块
6. 启动提示音（蜂鸣器短促一声）
7. LED 白色慢闪（初始化完成指示）
8. 注册 7 个串口命令
9. 打印 `> ` 提示符

**loop() 每轮执行**：
1. `cmd.update()` — 检查串口输入
2. `sensor.readSensor()` — 读取温湿度（内部计时）
3. `led.update()` — LED 闪烁更新
4. `buzzer.update()` — 蜂鸣器报警更新
5. `buttons.update()` — 按钮状态检测
6. 滞回控制逻辑（湿度 < 45% 开，> 55% 关）
7. LED 状态映射（绿=正常/蓝=加湿/红=异常）
8. 按钮事件处理（单击/双击/长按）
9. `display.update()` — 刷新 OLED
10. 每 30 秒输出心跳日志

---

## 📂 相关笔记

- [[SHC-项目主页]] — 返回项目总览
- [[SHC-固件架构详解]] — 架构设计文档
- [[SHC-开发日志]] — 每日开发记录
- [[SHC-编译与Bug记录]] — 编译记录和 Bug 分析
