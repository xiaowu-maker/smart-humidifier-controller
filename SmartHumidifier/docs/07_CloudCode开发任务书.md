# 07 — Cloud Code 开发任务书

**项目**：Smart Humidifier Controller (SHC)
**版本**：V1.0
**用途**：每个任务可直接复制给 Cloud Code / Claude Code 执行
**总任务数**：45 个
**预计总工时**：约 40 小时

---

## 📋 任务总览

```
Phase 1  ████████░░░░░░░░  Task 001-008  开发环境与第一个程序
Phase 2  ████████░░░░░░░░  Task 009-014  传感器驱动
Phase 3  ████████░░░░░░░░  Task 015-020  OLED显示
Phase 4  ████████░░░░░░░░  Task 021-028  自动控制
Phase 5  ████████░░░░░░░░  Task 029-036  WiFi联网+Web (V2.0)
Phase 6  ████████░░░░░░░░  Task 037-041  OTA+优化
Phase 7  ████████░░░░░░░░  Task 042-045  测试与发布
```

---

# Phase 1：开发环境与第一个程序

---

## Task 001 — 创建 PlatformIO 工程

### 🎯 任务目标
创建一个可编译的 ESP32 PlatformIO 工程。

### 📖 背景说明
PlatformIO 是 ESP32 开发的标配工具。它比 Arduino IDE 更适合多人协作和模块化开发。这一步是整个项目的基石。

### 🤖 给 Cloud Code 的提示词

```
请帮我创建一个 ESP32 的 PlatformIO 工程。

要求：
1. 项目名称：SmartHumidifier
2. 开发板：ESP32 DevKit V1 (esp32dev)
3. 框架：Arduino
4. 工程目录结构：
   SmartHumidifier/
   ├── platformio.ini
   ├── src/
   │   └── main.cpp
   ├── include/
   │   └── config.h
   ├── lib/
   ├── test/
   └── README.md

5. platformio.ini 配置：
   - 波特率 115200
   - 开启编译优化 -Os
   - 监控串口过滤（只显示重要日志）

6. main.cpp 中写一个最小程序：
   - 初始化串口 115200
   - 打印 "Smart Humidifier Controller V1.0"
   - 打印 "System boot complete."

7. config.h 中预留：
   - 项目名称、版本号宏定义
   - 串口波特率宏定义

请输出完整的文件内容。
```

### 👤 你需要做的操作
1. 打开 VS Code
2. 确保 PlatformIO 插件已安装
3. 在终端中进入项目目录
4. 把 Cloud Code 生成的文件内容复制到对应位置

### ✅ 验收标准
- [ ] `platformio.ini` 文件存在且语法正确
- [ ] `src/main.cpp` 文件存在
- [ ] `include/config.h` 文件存在
- [ ] 点击 PlatformIO 的 Build（√）按钮，编译成功
- [ ] 终端无错误输出

### 🐞 常见问题与排查

| 问题 | 可能原因 | 解决方法 |
|------|----------|----------|
| 找不到 platformio 命令 | PlatformIO 未安装 | VS Code 扩展商店搜索安装 PlatformIO IDE |
| 编译报错 "board not found" | 开发板配置错误 | 确认 board = esp32dev |
| 中文注释乱码 | 文件编码问题 | VS Code 右下角改为 UTF-8 |

---

## Task 002 — 配置 ESP32 开发环境

### 🎯 任务目标
确认 PlatformIO 能正确识别 ESP32 开发板并完成首次编译。

### 📖 背景说明
ESP32 DevKit V1 是最常用的 ESP32 开发板。PlatformIO 中它的 ID 是 `esp32dev`。这一步确保工具链和硬件匹配。

### 🤖 给 Cloud Code 的提示词

```
请帮我完善 SmartHumidifier 项目的 platformio.ini 配置文件。

当前项目使用：
- 开发板：ESP32 DevKit V1 (esp32dev)
- 框架：Arduino
- 波特率：115200

需要添加：
1. 编译时开启所有警告 (-Wall -Wextra)
2. 优化等级：-Os (尺寸优化)
3. 添加以下库依赖：
   - adafruit/Adafruit AHTX0 @ ^2.0.5
   - adafruit/Adafruit SSD1306 @ ^2.5.7
   - adafruit/Adafruit GFX Library @ ^1.11.9
   - adafruit/Adafruit BusIO @ ^1.14.5
   - bblanchon/ArduinoJson @ ^7.0.4
4. 设置 Flash 大小为 4MB
5. 设置 Partition Scheme 为 Default 4MB with spiffs

请输出完整的 platformio.ini 文件内容。
```

### 👤 你需要做的操作
1. 把生成的 platformio.ini 覆盖到项目中
2. 点击 Build，PlatformIO 会自动下载依赖库
3. 等待编译完成，确认无错误

### ✅ 验收标准
- [ ] 编译成功
- [ ] 库依赖自动下载完成
- [ ] 固件大小合理（< 500KB）

### 🐞 常见问题与排查

| 问题 | 可能原因 | 解决方法 |
|------|----------|----------|
| 库下载失败 | 网络问题 | 更换 pip 源或手动下载 |
| 编译报错找不到库 | 库名拼写错误 | 在 PlatformIO 库管理器中搜索确认 |
| 固件过大 | 包含了不必要的库 | 检查 lib_deps 是否多余 |

---

## Task 003 — 烧录第一个程序

### 🎯 任务目标
将编译好的固件烧录到 ESP32 开发板上。

### 📖 背景说明
ESP32 通过 USB 转串口芯片（CP2102 或 CH340）与电脑通信。烧录前需要安装对应的 USB 驱动。

### 🤖 给 Cloud Code 的提示词

```
帮我完善 main.cpp，让它在烧录后能够验证硬件基本功能。

要求：
1. 在 setup() 中：
   - 初始化串口（使用 config.h 中的宏定义）
   - 等待 1 秒让串口稳定
   - 打印分隔线（等号组成的线）
   - 打印项目名称和版本号
   - 打印 "MCU: ESP32 DevKit V1"
   - 打印 "Flash: 4MB"
   - 打印 "Freq: 240MHz"
   - 打印分隔线
   - 打印 "Initializing..."

2. 在 loop() 中：
   - 打印 "System OK. Uptime: X seconds"（X 为实际运行秒数）
   - 每 5 秒打印一次
   - 使用 millis() 计算运行时间，不要用 delay()

3. config.h 中的宏定义：
   #define PROJECT_NAME "Smart Humidifier Controller"
   #define PROJECT_VERSION "1.0.0"
   #define SERIAL_BAUDRATE 115200

请输出 main.cpp 和 config.h 的完整内容。
```

### 👤 你需要做的操作
1. 用 USB 数据线连接 ESP32 到电脑
2. 检查设备管理器是否识别到串口（COM端口）
3. 如果没识别，安装 CH340 或 CP2102 驱动
4. 在 VS Code 底部状态栏点击 "→" (Upload) 按钮烧录
5. 打开串口监视器（波特率 115200）

### ✅ 验收标准
- [ ] 烧录成功（终端显示 "SUCCESS"）
- [ ] 串口监视器看到启动信息
- [ ] 每 5 秒打印运行时间
- [ ] ESP32 上的电源 LED 亮起

### 🐞 常见问题与排查

| 问题 | 可能原因 | 解决方法 |
|------|----------|----------|
| 找不到串口 | 驱动未安装 | 下载 CH340/CP2102 驱动 |
| 烧录失败 "Connecting..." | 进入了下载模式失败 | 按住 BOOT 按钮再按 EN 按钮 |
| 串口显示乱码 | 波特率不匹配 | 确认监视器波特率为 115200 |
| 烧录后无输出 | 串口监视器没打开 | 点击底部插头图标 |

---

## Task 004 — 串口输出 "Hello Smart Humidifier"

### 🎯 任务目标
建立规范的串口日志系统，替代简单的 print 语句。

### 📖 背景说明
一个专业的嵌入式项目需要有规范的日志系统。日志能帮助你在设备出问题时快速定位原因。后续所有模块都将使用这个日志系统。

### 🤖 给 Cloud Code 的提示词

```
请为 SmartHumidifier 项目创建一个轻量级日志模块。

要求：
1. 创建 Logger.h 和 Logger.cpp，放在 src/ 目录

2. 定义 4 个日志级别：
   - DEBUG (0): 调试信息
   - INFO (1): 一般信息
   - WARN (2): 警告
   - ERROR (3): 错误

3. 提供宏定义函数（方便调用）：
   LOG_DEBUG(tag, msg)
   LOG_INFO(tag, msg)
   LOG_WARN(tag, msg)
   LOG_ERROR(tag, msg)

4. 日志输出格式：
   [LEVEL] [TAG] message
   
   示例：
   [INFO] [SYSTEM] Smart Humidifier Controller V1.0
   [WARN] [SENSOR] Read timeout, retrying...
   [ERROR] [SENSOR] Sensor disconnected!

5. 使用 Serial.print 输出（不依赖任何库）
6. 可以通过宏 SWITCH 设置最低日志级别（低于此级别不输出）
7. 添加一个启动横幅函数 printBanner()，输出项目名称和版本号

请输出 Logger.h, Logger.cpp 和修改后的 main.cpp（使用日志系统）。
```

### 👤 你需要做的操作
1. 把生成的 Logger.h 和 Logger.cpp 放入 src/ 目录
2. 替换 main.cpp
3. 编译、烧录
4. 打开串口监视器观察日志输出

### ✅ 验收标准
- [ ] 编译成功
- [ ] 串口输出符合 `[LEVEL] [TAG] message` 格式
- [ ] 启动时看到横幅信息
- [ ] 日志级别显示正确

### 🐞 常见问题与排查

| 问题 | 可能原因 | 解决方法 |
|------|----------|----------|
| 编译错误 "Logger.h not found" | 文件位置不对 | 确保放在 src/ 目录，与 main.cpp 同级 |
| 日志不输出 | 日志级别过滤 | 检查 LOG_LEVEL 宏定义 |

---

## Task 005 — 项目目录结构完善

### 🎯 任务目标
按照真实的软件工程标准，完善项目目录结构。

### 📖 背景说明
好的目录结构让项目易于维护和扩展。按照手册第十章的建议建立完整目录。

### 🤖 给 Cloud Code 的提示词

```
请为 SmartHumidifier 项目规划完整的目录结构。

当前项目已有：
- firmware/ 目录（PlatformIO 工程）

请帮我创建以下目录和文件结构：

SmartHumidifier/
├── docs/                          # 文档目录
├── hardware/                      # 硬件相关
│   ├── schematic/                 # 原理图
│   └── bom.md                     # 物料清单
├── firmware/                      # 固件（已有的 PlatformIO 工程）
│   ├── platformio.ini
│   ├── src/                       # 源代码
│   │   ├── main.cpp
│   │   ├── Logger.h
│   │   ├── Logger.cpp
│   │   ├── config.h
│   │   ├── SensorManager.h
│   │   ├── SensorManager.cpp
│   │   ├── DisplayManager.h
│   │   ├── DisplayManager.cpp
│   │   ├── ActuatorManager.h
│   │   ├── ActuatorManager.cpp
│   │   ├── HumidifierController.h
│   │   ├── HumidifierController.cpp
│   │   ├── WiFiManager.h       (V2.0)
│   │   ├── WiFiManager.cpp     (V2.0)
│   │   ├── WebServerManager.h  (V2.0)
│   │   └── WebServerManager.cpp(V2.0)
│   ├── include/                   # 公共头文件
│   ├── data/                      # Web 页面文件 (SPIFFS)
│   └── test/                      # 单元测试
├── images/                        # 项目图片
├── tests/                         # 集成测试
├── README.md                      # 项目说明
├── CHANGELOG.md                   # 版本日志
└── LICENSE                        # 开源协议 (MIT)

请输出：
1. 创建所有空模块文件的指令（每个 .h/.cpp 包含文件头和包含保护宏）
2. README.md 的初始版本
3. CHANGELOG.md 的初始版本
4. LICENSE 文件（MIT 协议）
```

### 👤 你需要做的操作
1. 在项目根目录下创建 docs/、hardware/、images/、tests/ 目录
2. 创建所有模块的空壳文件
3. 填写 README.md 和 LICENSE

### ✅ 验收标准
- [ ] 目录结构如上所示
- [ ] 所有 .h/.cpp 文件包含正确的头文件保护宏
- [ ] README.md 包含项目简介
- [ ] LICENSE 文件存在

---

## Task 006 — 创建 config.h 完整配置

### 🎯 任务目标
将项目中所有可配置参数集中到一个文件，做到改配置不改代码。

### 📖 背景说明
config.h 是项目的"控制面板"。所有引脚定义、阈值参数、WiFi 凭证都在这里。换一个引脚只需要改一行，不需要到处找代码。

### 🤖 给 Cloud Code 的提示词

```
请为 SmartHumidifier 项目编写完整的 config.h 配置文件。

要求：
1. 项目信息宏定义：
   - PROJECT_NAME
   - PROJECT_VERSION
   - SERIAL_BAUDRATE

2. GPIO 引脚定义（使用注释说明每个引脚的功能）：
   - I2C: SDA=GPIO21, SCL=GPIO22
   - 继电器: GPIO4 (低电平触发)
   - RGB LED: R=GPIO16, G=GPIO17, B=GPIO18 (共阴极，高电平点亮)
   - 蜂鸣器: GPIO19 (高电平发声)
   - 按钮A: GPIO13 (模式切换，内部上拉)
   - 按钮B: GPIO14 (恢复默认，内部上拉)

3. 控制参数：
   - HUMIDITY_LOW_THRESHOLD 45.0f  // 低于此值开启加湿器
   - HUMIDITY_HIGH_THRESHOLD 55.0f // 高于此值关闭加湿器
   - SENSOR_READ_INTERVAL_MS 2000  // 传感器读取间隔
   - DISPLAY_UPDATE_INTERVAL_MS 2000 // 显示刷新间隔
   - RELAY_MIN_ON_TIME_MS 60000    // 继电器最短运行时间
   - RELAY_DEBOUNCE_COUNT 3        // 防抖采样次数

4. WiFi 配置（V2.0 使用）：
   - WIFI_SSID 占位符
   - WIFI_PASSWORD 占位符
   - WIFI_RECONNECT_INTERVAL_MS 30000

5. 每个宏定义上方都要有注释说明用途

请输出完整的 config.h 文件。
```

### 👤 你需要做的操作
1. 把生成的 config.h 覆盖到 include/ 目录
2. 检查引脚分配是否符合你的实际接线

### ✅ 验收标准
- [ ] config.h 包含所有必要的宏定义
- [ ] 每个宏定义都有注释
- [ ] 编译成功

---

## Task 007 — GPIO 基础测试

### 🎯 任务目标
验证所有 GPIO 引脚功能正常，为后续模块开发打基础。

### 📖 背景说明
在连接传感器之前，先用简单的 GPIO 测试确认开发板各引脚工作正常。这是硬件调试的"Hello World"。

### 🤖 给 Cloud Code 的提示词

```
请为 SmartHumidifier 项目编写一个 GPIO 自检程序。

要求：
1. 创建一个 GpioTest.h / GpioTest.cpp 模块

2. 自检功能：
   - LED 测试：依次点亮红、绿、蓝各 500ms，然后全灭
   - 蜂鸣器测试：短促响 3 声（每声 200ms）
   - 继电器测试：吸合 1 秒，断开 1 秒（请先确认没有接高压设备！）
   - 按钮测试：检测两个按钮是否能正常读取（串口输出按键状态）
   - I2C 总线扫描：扫描 I2C 总线上的设备，列出所有从设备地址

3. main.cpp 中：
   - 启动后先运行 GPIO 自检
   - 自检完成后进入正常模式
   - 按 Button A 可重新运行自检

4. 所有测试结果通过串口日志输出

请输出 GpioTest.h, GpioTest.cpp 和修改后的 main.cpp。
```

### 👤 你需要做的操作
1. 先不要接继电器（安全第一！）
2. 接上三色 LED 和蜂鸣器到面包板
3. 编译、烧录
4. 观察 LED 闪烁和蜂鸣器声音
5. 按按钮观察串口输出

### ✅ 验收标准
- [ ] RGB LED 依次亮起红、绿、蓝
- [ ] 蜂鸣器发出 3 声短促声音
- [ ] 串口显示按钮状态
- [ ] I2C 扫描结果输出到串口

### 🐞 常见问题与排查

| 问题 | 可能原因 | 解决方法 |
|------|----------|----------|
| LED 不亮 | 正负极接反 | 共阴极 LED 的公共脚接 GND |
| 蜂鸣器不响 | 需要三极管驱动 | 有源蜂鸣器可直接接 GPIO（电流 < 20mA） |
| 按钮读数不变 | 未接上拉电阻 | 代码中使用 INPUT_PULLUP |

---

## Task 008 — 建立编译与烧录检查清单

### 🎯 任务目标
建立标准化的编译和烧录流程，避免因环境问题浪费时间。

### 📖 背景说明
很多初学者的问题不是代码错误，而是编译环境配置问题。建立一个检查清单可以快速定位问题。

### 🤖 给 Cloud Code 的提示词

```
请帮我创建一个编译检查清单文档，放到 docs/ 目录下。

文档名：docs/附录_编译烧录检查清单.md

内容：
1. 环境检查（Python版本、PlatformIO版本、Git状态）
2. 编译前检查（platformio.ini 配置、库依赖完整性）
3. 编译常见错误及解决（10个以上常见错误）
4. 烧录前检查（USB线是否支持数据传输、COM端口确认）
5. 烧录常见错误及解决（5个以上常见错误）
6. 烧录后验证步骤
7. 快速诊断流程图（用文本画出决策树）

请输出完整文档。
```

### 👤 你需要做的操作
1. 阅读并理解检查清单
2. 以后遇到问题先查这份文档

### ✅ 验收标准
- [ ] 文档创建完成
- [ ] 包含至少 10 个编译错误和解决方案
- [ ] 包含至少 5 个烧录错误和解决方案

---

> **Phase 1 完成！此时你应该有一块能跑程序的 ESP32，串口正常输出日志。**

---

# Phase 2：传感器驱动

---

## Task 009 — AHT20 传感器接线

### 🎯 任务目标
正确连接 AHT20 传感器到 ESP32 的 I2C 总线。

### 📖 背景说明
AHT20 是新一代温湿度传感器，精度高、体积小，通过 I2C 通信。它只需要 4 根线：VCC、GND、SDA、SCL。

### 🤖 给 Cloud Code 的提示词

```
请帮我写一份 AHT20 接线指南，放入 docs/ 目录。

文档名：docs/附录_AHT20接线指南.md

内容：
1. AHT20 模块引脚说明（正面图标注）
2. 接线表（ESP32 ↔ AHT20）
3. 完整接线图（ASCII art）
4. 注意事项：
   - 供电 3.3V（不是 5V！）
   - I2C 需要上拉电阻（一般模块已内置）
   - 不要热插拔
5. 接线检查清单
6. 常见错误接线及后果

请输出完整文档。
```

### 👤 你需要做的操作
1. 按照接线指南连接 AHT20
2. 用万用表确认 VCC 和 GND 之间没有短路
3. 上电后用 Task 007 的 I2C 扫描功能确认 0x38 地址存在

### ✅ 验收标准
- [ ] I2C 扫描显示 0x38 设备
- [ ] AHT20 模块电源指示灯亮起

---

## Task 010 — SensorManager 类框架

### 🎯 任务目标
创建 SensorManager 类的基本框架，为 AHT20 驱动做准备。

### 📖 背景说明
按照软件架构设计文档，SensorManager 负责所有传感器相关操作。先定义接口，再实现具体功能，这是面向接口编程的核心思想。

### 🤖 给 Cloud Code 的提示词

```
请为 SmartHumidifier 项目创建 SensorManager 模块的完整框架。

按照 docs/04_软件架构设计.md 中的设计：

创建 SensorManager.h：
```cpp
class SensorManager {
public:
    bool begin();                    // 初始化传感器
    bool readSensor();               // 执行一次读取
    float getTemperature();          // 获取温度（℃）
    float getHumidity();             // 获取湿度（%RH）
    bool isSensorOK();               // 传感器状态
    const char* getErrorMsg();       // 错误信息
    unsigned long getLastReadTime(); // 上次读取时间
    
private:
    float _temperature;
    float _humidity;
    bool _sensorOK;
    unsigned long _lastReadTime;
    String _errorMsg;
    uint8_t _consecutiveErrors;      // 连续错误计数
};
```

创建 SensorManager.cpp：
- begin() 中初始化 I2C 和 AHT20
- 使用 Adafruit_AHTX0 库
- readSensor() 中包含错误处理和重试逻辑
- 读取失败时递增 _consecutiveErrors
- 读取成功后重置 _consecutiveErrors

要求：
- 完整的头文件保护宏
- 每个方法都有注释说明
- 使用 Logger 输出关键信息
- 错误时使用缓存值（上一次有效读数）

请输出 SensorManager.h 和 SensorManager.cpp 的完整内容。
```

### 👤 你需要做的操作
1. 将文件放入 firmware/src/ 目录
2. 在 main.cpp 中 include SensorManager.h
3. 编译确认无语法错误

### ✅ 验收标准
- [ ] 编译通过
- [ ] 头文件包含保护宏正确
- [ ] 每个方法有注释

---

## Task 011 — AHT20 驱动实现

### 🎯 任务目标
实现 AHT20 传感器的完整驱动程序。

### 📖 背景说明
AHT20 使用 I2C 通信，测量时间约 80ms。驱动需要处理初始化、触发测量、读取数据和 CRC 校验。

### 🤖 给 Cloud Code 的提示词

```
请完善 SensorManager.cpp 中的 AHT20 驱动实现。

要求：
1. begin() 实现：
   - 初始化 I2C 总线（使用 config.h 中的引脚定义）
   - 初始化 Adafruit_AHTX0 对象
   - 检测 AHT20 是否存在（尝试读取状态寄存器）
   - 成功返回 true，失败返回 false
   - 使用 LOG_INFO/LOG_ERROR 输出状态

2. readSensor() 实现：
   - 检查距离上次读取是否 >= SENSOR_READ_INTERVAL_MS
   - 调用 Adafruit_AHTX0 库读取温湿度
   - 如果读取失败，重试 1 次
   - 两次都失败 → _sensorOK = false，使用缓存值
   - 检查数据合理性（湿度 0-100%，温度 -40~80℃）
   - 超出范围视为异常
   - 更新 _lastReadTime
   - 使用 LOG_DEBUG 输出读取结果

3. 异常处理：
   - begin() 失败：LOG_ERROR "AHT20 not found at 0x38"
   - readSensor() 失败 1 次：LOG_WARN "Read failed, retrying..."
   - readSensor() 连续失败 3 次：LOG_ERROR "Sensor disconnected!"
   - 数值异常：LOG_WARN "Value out of range: X"

4. 不要使用 delay()，使用 millis() 进行非阻塞时间判断

请输出 SensorManager.cpp 完整内容。
```

### 👤 你需要做的操作
1. 替换 SensorManager.cpp
2. 编译、烧录
3. 打开串口监视器观察传感器数据

### ✅ 验收标准
- [ ] 串口每 2 秒输出温湿度数据
- [ ] 数据在合理范围内（温度 20-35℃，湿度 30-70%）
- [ ] 用手捂住传感器，湿度值上升

### 🐞 常见问题与排查

| 问题 | 可能原因 | 解决方法 |
|------|----------|----------|
| 一直显示初始化失败 | I2C 接线错误 | 检查 SDA/SCL 是否接反 |
| 温度显示 -50℃ | 传感器未正确初始化 | 检查 AHT20 供电 |
| 数据不变 | 传感器读取频率限制 | AHT20 最快 10Hz，已够用 |

---

## Task 012 — 传感器数据验证与校准

### 🎯 任务目标
增加数据验证逻辑，确保读取的数据可靠。

### 📖 背景说明
传感器偶尔会返回异常值（如湿度 200%）。需要增加验证逻辑过滤这些脏数据。同时预留校准接口，方便后续与标准仪器比对后校准。

### 🤖 给 Cloud Code 的提示词

```
请为 SensorManager 增加数据验证和校准功能。

要求：
1. 数据验证函数 validateData()：
   - 温度范围检查：-40℃ ~ 85℃（AHT20 规格范围）
   - 湿度范围检查：0% ~ 100%
   - 温度变化率检查：相邻两次读数温差不超过 10℃（防止跳变）
   - 湿度变化率检查：相邻两次读数湿度差不超过 20%
   - 返回 true 表示数据有效

2. 校准功能：
   - 增加 setTempOffset(float offset) 方法
   - 增加 setHumidityOffset(float offset) 方法
   - 读取时自动加上偏置值
   - 偏置值默认为 0

3. 传感器重启功能：
   - 增加 resetSensor() 方法
   - 软复位 AHT20（发送复位指令）
   - 连续 5 次读取失败后自动调用

4. 统计功能（调试用）：
   - getReadCount()：总读取次数
   - getErrorCount()：错误次数
   - getErrorRate()：错误率百分比

5. 日志输出改进：
   - 读取成功次数每 60 次打印一次统计信息
   - 格式：[INFO] [SENSOR] Stats: 120 reads, 3 errors (2.5%)

请输出改进后的 SensorManager.h 和 SensorManager.cpp。
```

### 👤 你需要做的操作
1. 替换文件，编译烧录
2. 观察数据是否稳定
3. 尝试拔掉传感器，观察错误处理

### ✅ 验收标准
- [ ] 数据无明显跳变
- [ ] 拔掉传感器后系统进入错误状态
- [ ] 插回传感器后自动恢复
- [ ] 串口每 60 次输出统计信息

---

## Task 013 — 传感器单元测试

### 🎯 任务目标
编写并运行传感器模块的单元测试。

### 📖 背景说明
单元测试是保证代码质量的重要手段。虽然嵌入式测试比软件测试复杂，但我们可以用串口输出验证关键功能。

### 🤖 给 Cloud Code 的提示词

```
请为 SensorManager 模块创建测试程序。

创建文件：firmware/test/test_sensor.cpp

测试内容：
1. TEST 1：初始化测试
   - 调用 begin() 应该返回 true

2. TEST 2：读取测试
   - 调用 readSensor() 应该返回 true
   - 温度和湿度在合理范围内

3. TEST 3：数据一致性测试
   - 连续读取 10 次
   - 相邻两次读数差异在合理范围内

4. TEST 4：异常恢复测试
   - 模拟传感器断开后的行为（通过软件复位传感器）
   - 错误计数应增加
   - 复位后应恢复

5. TEST 5：校准测试
   - 设置温度偏移 +1.0℃
   - 读取温度应比原始值高 1.0℃

6. TEST 6：边界值测试
   - 验证极端温度值（-40℃, 85℃）不会导致崩溃
   - 验证极端湿度值（0%, 100%）不会导致崩溃

测试结果格式：
====================
SensorManager Tests
====================
[PASS] TEST 1: Initialization
[PASS] TEST 2: Data reading
[FAIL] TEST 3: Data consistency - variation too large (15.3%)
...
--------------------
Result: 5/6 tests passed
====================

请输出 test_sensor.cpp 完整内容。
```

### 👤 你需要做的操作
1. 将测试文件放入 firmware/test/
2. 在 platformio.ini 中配置测试环境
3. 编译并烧录测试程序
4. 观察测试结果

### ✅ 验收标准
- [ ] 所有核心测试通过
- [ ] 测试输出格式清晰

---

## Task 014 — Phase 2 集成测试

### 🎯 任务目标
将 SensorManager 集成到主程序，确认整体工作正常。

### 📖 背景说明
模块单独工作正常不代表集成后也正常。这一步将传感器模块集成到主循环，为后续开发做准备。

### 🤖 给 Cloud Code 的提示词

```
请修改 main.cpp，将 SensorManager 集成到主循环。

要求：
1. setup() 中：
   - 初始化串口和日志系统
   - 打印启动横幅
   - 初始化 SensorManager
   - 如果传感器初始化失败，LED 显示红色，但不阻止系统运行

2. loop() 中：
   - 每 2 秒读取一次传感器
   - 格式化为一行输出：
     [INFO] [MAIN] Temp: 26.3°C | Humidity: 48.2% | Sensor: OK | Uptime: 120s
   - 如果传感器异常，输出警告

3. 使用 millis() 实现非阻塞循环（不用 delay()）

4. 添加一个简单的命令解析器：
   - 从串口读取命令
   - 支持命令：STATUS, RESET, HELP
   - STATUS → 输出当前传感器数据和系统状态
   - RESET → 软件重启
   - HELP → 显示可用命令

请输出修改后的 main.cpp。
```

### 👤 你需要做的操作
1. 替换 main.cpp，编译烧录
2. 观察串口输出是否稳定
3. 尝试在串口监视器中输入 STATUS 命令

### ✅ 验收标准
- [ ] 串口每 2 秒输出传感器数据
- [ ] 数据格式正确
- [ ] 串口命令 STATUS 有正确响应
- [ ] 传感器断开时有告警输出

---

> **Phase 2 完成！ESP32 能稳定读取温湿度数据，串口命令可交互。**

---

*(接下页...)*

---

# Phase 3：OLED 显示

---

## Task 015 — OLED 接线

### 🎯 任务目标
正确连接 0.96 寸 OLED 到 ESP32 的 I2C 总线。

### 📖 背景说明
0.96 寸 OLED 使用 SSD1306 驱动芯片，与 AHT20 共享 I2C 总线。因为 I2C 地址不同（OLED: 0x3C, AHT20: 0x38），可以挂在同一总线上。

### 🤖 给 Cloud Code 的提示词

```
请帮我写一份 OLED 接线指南，放入 docs/ 目录。

文档名：docs/附录_OLED接线指南.md

内容：
1. 0.96寸 OLED 模块引脚说明
   - VCC (3.3V)
   - GND
   - SCL (I2C 时钟)
   - SDA (I2C 数据)

2. 共享 I2C 总线接线图（ESP32 + AHT20 + OLED 三者的连接）
   - 用 ASCII art 画出面包板布局
   - 标出所有 VCC、GND、SDA、SCL 连接

3. I2C 地址确认
   - AHT20：0x38
   - OLED：0x3C
   - 说明两个地址不冲突

4. 上电检查
   - I2C 扫描应看到 0x38 和 0x3C 两个设备

请输出完整文档。
```

### 👤 你需要做的操作
1. 在面包板上添加 OLED 模块
2. SDA 和 SCL 并联到已有的 AHT20 线上
3. VCC 接 3.3V，GND 接 GND
4. 用 I2C 扫描确认 0x3C 地址存在

### ✅ 验收标准
- [ ] I2C 扫描同时显示 0x38 和 0x3C
- [ ] OLED 上电后屏幕亮起（可能有杂乱像素）

---

## Task 016 — DisplayManager 类框架

### 🎯 任务目标
创建 DisplayManager 类，封装 OLED 操作。

### 📖 背景说明
DisplayManager 负责所有屏幕显示逻辑。遵循单一职责原则，其他模块不应直接操作 OLED。

### 🤖 给 Cloud Code 的提示词

```
请为 SmartHumidifier 项目创建 DisplayManager 模块。

按照 docs/04_软件架构设计.md 中的设计：

DisplayManager.h 接口：
```cpp
class DisplayManager {
public:
    bool begin();
    void update(float temp, float humidity, bool relayOn, const char* status);
    void showError(const char* msg);
    void showInitProgress(const char* step);  // 启动时显示初始化进度
    void clear();
    void nextPage();                          // 切换显示页面
    uint8_t getCurrentPage();
    
private:
    Adafruit_SSD1306 _display;    // I2C OLED对象
    uint8_t _currentPage;
    bool _initialized;
    
    void _drawMainPage(float temp, float humidity, bool relayOn);
    void _drawNetworkPage();
    void _drawSystemPage();
};
```

DisplayManager.cpp 实现：
- begin()：初始化 SSD1306（地址0x3C, 128x64）
- update()：根据当前页面调用对应的绘制函数
- 使用 Adafruit_SSD1306 和 Adafruit_GFX 库
- 如果初始化失败，_initialized = false，所有绘制操作跳过

请输出 DisplayManager.h 和 DisplayManager.cpp 的完整内容。
```

### 👤 你需要做的操作
1. 将文件放入 firmware/src/
2. 编译确认无错误

### ✅ 验收标准
- [ ] 编译通过
- [ ] 类接口与架构设计一致

---

## Task 017 — OLED 基本显示

### 🎯 任务目标
在 OLED 上显示文字内容。

### 📖 背景说明
SSD1306 OLED 通过 Adafruit_GFX 库支持多种字体和绘图功能。先实现最基本的文字显示。

### 🤖 给 Cloud Code 的提示词

```
请实现 DisplayManager 的基本显示功能。

要求：
1. begin() 实现：
   - Wire.begin(SDA_PIN, SCL_PIN)
   - display.begin(SSD1306_SWITCHCAPVCC, 0x3C)
   - display.clearDisplay()
   - display.setTextColor(SSD1306_WHITE)
   - display.display()
   - 初始化成功/失败输出日志

2. showInitProgress() 实现：
   - 在屏幕中央显示 "Smart Humidifier"
   - 下方显示当前初始化步骤
   - 例如："Initializing OLED...  OK"

3. clear() 实现：
   - 清屏

4. _drawMainPage() 实现（核心显示）：
   - 屏幕布局（128x64像素）：

   ┌──────────────────────┐  y=0
   │ Smart Humidifier     │  y=8   (size 1, 标题)
   ├──────────────────────┤  y=16 (分割线)
   │ Humidity : 48.2%     │  y=24  (size 2, 湿度)
   │ Temp    : 26.3 C     │  y=40  (size 2, 温度)
   │ Status  : HEATING    │  y=56  (size 1, 状态)
   └──────────────────────┘  y=63

   - 标题：小字体，居中
   - 湿度和温度：大字体（size 2），左对齐
   - 状态行：小字体
   - 继电器 ON 时显示 "Status  : HEATING"
   - 继电器 OFF 时显示 "Status  : STANDBY"

5. 使用 millis() 控制刷新频率（不超过 DISPLAY_UPDATE_INTERVAL_MS）

请输出完整的 DisplayManager.cpp。
```

### 👤 你需要做的操作
1. 替换文件，编译烧录
2. OLED 应该显示启动画面
3. 集成到主循环后应显示温湿度

### ✅ 验收标准
- [ ] OLED 显示 "Smart Humidifier" 标题
- [ ] 温湿度数据显示正确
- [ ] 状态行根据继电器状态变化

---

## Task 018 — OLED 多页面切换

### 🎯 任务目标
实现多页面显示功能，通过按钮切换。

### 📖 背景说明
OLED 屏幕很小，需要分页显示不同类型的信息。用 Button A 在页面间切换。

### 🤖 给 Cloud Code 的提示词

```
请为 DisplayManager 实现多页面切换功能。

要求：
1. 3个显示页面：

Page 0 — 主页面（默认）：
┌──────────────────────┐
│ Smart Humidifier     │
│ Humidity : 48.2%     │
│ Temp    : 26.3 C     │
│ Status  : HEATING    │
└──────────────────────┘

Page 1 — 网络信息页：
┌──────────────────────┐
│ Network Info         │
│ IP: 192.168.1.100    │
│ RSSI: -45 dBm        │
│ WiFi: Connected      │
└──────────────────────┘

Page 2 — 系统信息页：
┌──────────────────────┐
│ System Info          │
│ Ver: V1.0.0          │
│ Uptime: 2h 15m       │
│ Sensor: OK           │
│ Reads: 3600          │
└──────────────────────┘

2. nextPage() 方法：
   - 当前页号 +1
   - 超过最大值回绕到 0
   - 切换时短暂清屏（防止残影）

3. 页面指示器：
   - 底部显示圆点指示器，如 "● ○ ○" 表示在第1页

4. 每页独立绘制函数：
   - _drawMainPage()
   - _drawNetworkPage()
   - _drawSystemPage()

5. update() 方法需要接收所有页面的数据：
   - update(temp, humidity, relayOn, status, ip, rssi, uptime, sensorOK, readCount)

请输出改进后的 DisplayManager.h 和 DisplayManager.cpp。
```

### 👤 你需要做的操作
1. 替换文件，编译烧录
2. 用跳线将 Button A 接到 GPIO13
3. 按按钮观察页面切换

### ✅ 验收标准
- [ ] 3个页面显示正常
- [ ] 按钮切换页面正常
- [ ] 底部页面指示器正确

---

## Task 019 — OLED 显示优化

### 🎯 任务目标
优化显示质量，让屏幕看起来更专业。

### 📖 背景说明
默认字体和布局不够美观。通过优化布局、添加图标、使用自定义字符等方式提升显示品质。

### 🤖 给 Cloud Code 的提示词

```
请优化 DisplayManager 的显示效果。

要求：
1. 布局优化：
   - 湿度值添加动态状态图标：
     湿度 < 45%: 显示 "💧" (用两个小圆点模拟)
     湿度 45-55%: 显示 "✓"
     湿度 > 55%: 显示 "▲"

2. 视觉效果：
   - 标题行反白显示（白底黑字）
   - 分割线用 drawFastHLine

3. 动画效果：
   - 加热状态时在状态行末尾显示呼吸动画 "."
   - 每 500ms 切换 " . .. ..."

4. 字体混合：
   - 数值使用大字体（setTextSize(2)）
   - 标签和单位使用小字体（setTextSize(1)）

5. 无闪烁刷新：
   - 使用全屏缓冲区刷新（display.display()）
   - 不要在循环中多次调用 display()

6. 屏幕保护：
   - 10分钟无按钮操作后亮度降低
   - 实际上通过降低刷新率实现（从2秒改为10秒）
   - 按任意按钮恢复

请输出优化后的 DisplayManager.cpp。
```

### 👤 你需要做的操作
1. 替换文件，编译烧录
2. 观察显示效果
3. 等待 10 分钟观察屏幕保护效果

### ✅ 验收标准
- [ ] 显示效果比之前更美观
- [ ] 动画效果正常
- [ ] 屏幕保护正常工作

---

## Task 020 — Display 单元测试

### 🎯 任务目标
验证 DisplayManager 的所有显示页面。

### 📖 背景说明
OLED 显示无法像纯软件一样自动化测试，但可以通过固定的测试序列人工验证。

### 🤖 给 Cloud Code 的提示词

```
请创建一个 DisplayManager 显示测试程序。

文件：firmware/test/test_display.cpp

测试序列（人工验证）：
1. 全屏测试：依次显示全白、全黑、棋盘格
2. 文字测试：显示测试字符串 "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789"
3. 页面测试：自动循环显示 3 个页面，每页停留 3 秒
4. 动画测试：显示进度条从 0% 到 100%
5. 边界测试：显示坐标 (0,0) 和 (127,63) 的像素点

测试程序提示用户观察结果：
Serial.println("[TEST] Full white screen - You should see a white screen");
delay(2000);
...

最后询问用户：
Serial.println("Did all tests pass? (y/n)");

请输出 test_display.cpp。
```

### 👤 你需要做的操作
1. 烧录测试程序
2. 按串口提示观察 OLED 显示
3. 在串口输入 y/n 反馈结果

### ✅ 验收标准
- [ ] 所有测试页面显示正常
- [ ] 无残影、花屏现象

---

> **Phase 3 完成！OLED 实时显示温湿度数据，多页面切换正常。**

---

# Phase 4：自动控制

---

## Task 021 — 继电器驱动

### 🎯 任务目标
实现继电器控制模块，能够安全地控制加湿器开关。

### 📖 背景说明
继电器是控制 220V 加湿器的关键元件。ESP32 的 GPIO 输出 3.3V，继电器模块自带驱动电路。**安全警告：继电器接高压设备时务必断电操作！**

### 🤖 给 Cloud Code 的提示词

```
请为 SmartHumidifier 实现继电器控制模块。

要求：
1. 独立的 RelayManager 类（合并到 ActuatorManager 中也可）：

```cpp
class RelayManager {
public:
    void begin(uint8_t pin, bool activeHigh);
    void turnOn();
    void turnOff();
    void toggle();
    bool isOn();
    unsigned long getLastChangeTime();  // 上次状态变化时间
    
private:
    uint8_t _pin;
    bool _activeHigh;       // true=高电平触发, false=低电平触发
    bool _state;            // 当前状态
    unsigned long _lastChangeTime;
};
```

2. 安全特性：
   - 初始化时默认关闭（安全第一）
   - turnOn() 前检查距上次变化是否 >= RELAY_MIN_ON_TIME_MS
   - 如果时间不够，拒绝操作并 LOG_WARN
   - 增加 turnOnForce() 方法（跳过时间检查，仅紧急情况用）

3. 操作日志：
   - 每次状态变化都记录 LOG_INFO
   - 格式：[INFO] [RELAY] Relay ON (Humidity: 44.5%)
   - 格式：[INFO] [RELAY] Relay OFF (Humidity: 55.2%)

4. 统计信息：
   - getTotalOnTime()：累计开启时间
   - getCycleCount()：开关次数
   - getTodayOnCount()：今日开关次数

5. config.h 中的配置：
   #define RELAY_PIN 4
   #define RELAY_ACTIVE_HIGH false  // 低电平触发
   #define RELAY_MIN_ON_TIME_MS 60000
   #define RELAY_MIN_OFF_TIME_MS 30000

请输出 RelayManager.h 和 RelayManager.cpp（或 ActuatorManager 中包含 RelayManager）。
```

### 👤 你需要做的操作
1. 先不要接加湿器！
2. 接上继电器模块（VCC→5V, GND→GND, IN→GPIO4）
3. 用万用表蜂鸣档测试继电器的 NO/COM 通断
4. 听到继电器吸合/断开的声音

### ✅ 验收标准
- [ ] 继电器能正常吸合/断开
- [ ] 听到清晰的"咔哒"声
- [ ] 默认上电状态为断开
- [ ] 最短运行时间保护生效

---

## Task 022 — LED 状态管理

### 🎯 任务目标
实现三色 LED 的状态指示功能。

### 📖 背景说明
LED 是用户了解设备状态的最直观方式。三种颜色对应三种状态，简单明了。

### 🤖 给 Cloud Code 的提示词

```
请实现 LED 状态管理模块。

要求：
1. LedManager 类：

```cpp
enum class LedColor { 
    OFF,
    GREEN,   // 正常运行
    BLUE,    // 正在加湿
    RED,     // 传感器异常
    YELLOW,  // 警告（如WiFi断开）
    WHITE    // 初始化中
};

enum class LedPattern {
    SOLID,      // 常亮
    SLOW_BLINK, // 慢闪（1秒周期）
    FAST_BLINK, // 快闪（0.3秒周期）
    DOUBLE_BLINK // 双闪
};

class LedManager {
public:
    void begin(uint8_t pinR, uint8_t pinG, uint8_t pinB);
    void setColor(LedColor color);
    void setPattern(LedColor color, LedPattern pattern);
    void update();  // 在loop中调用，处理闪烁逻辑
    void turnOff();
    
private:
    // 非阻塞闪烁实现
    void _applyColor(LedColor color);
    LedColor _currentColor;
    LedPattern _currentPattern;
    bool _ledState;
    unsigned long _lastToggleTime;
    int _blinkPhase;
};
```

2. 状态与颜色映射：
   INIT → WHITE 慢闪
   RUNNING → GREEN 常亮
   HEATING → BLUE 常亮
   SENSOR_ERROR → RED 快闪
   WIFI_ERROR → YELLOW 慢闪

3. 非阻塞实现：
   - update() 使用 millis() 计算时间
   - 不使用 delay()
   - 闪烁频率精确

4. LED 亮度：
   - 使用 PWM 控制亮度
   - 正常亮度：200/255
   - 夜间模式（通过配置开启）：50/255

请输出 LedManager.h 和 LedManager.cpp。
```

### 👤 你需要做的操作
1. 接上 RGB LED（共阴极，公共脚接 GND，R→GPIO16, G→GPIO17, B→GPIO18）
2. 编译烧录
3. 观察不同状态下的 LED 颜色

### ✅ 验收标准
- [ ] 三种颜色都能正常亮起
- [ ] 闪烁模式正常
- [ ] 颜色切换无延迟

---

## Task 023 — 蜂鸣器报警

### 🎯 任务目标
实现蜂鸣器的报警功能。

### 📖 背景说明
蜂鸣器用于提醒用户注意异常情况。使用有源蜂鸣器（内置振荡器），通电即响，控制简单。

### 🤖 给 Cloud Code 的提示词

```
请实现蜂鸣器管理模块。

要求：
1. BuzzerManager 类：

```cpp
class BuzzerManager {
public:
    void begin(uint8_t pin);
    
    // 简单提示音
    void beep(int durationMs = 200);
    void doubleBeep();        // 两声短促的提示音
    
    // 报警模式
    void alarmSensorError();   // 传感器故障：间歇长鸣
    void alarmWifiLost();      // WiFi断开：三短一长
    void alarmCritical();      // 严重故障：连续急促鸣叫
    
    void stop();               // 停止所有声音
    void update();             // 非阻塞更新
    bool isActive();           // 是否正在发声
    
    // 静音控制
    void mute();               // 静音
    void unmute();             // 取消静音
    bool isMuted();
    
private:
    uint8_t _pin;
    bool _active;
    bool _muted;
    int _pattern;              // 当前报警模式
    int _patternStep;          // 当前模式步骤
    unsigned long _lastToggleTime;
};
```

2. 报警模式详情：
   - 传感器故障：响500ms，停1000ms，循环
   - WiFi断开：响100ms×3次（间隔100ms），停2000ms
   - 严重故障：响100ms，停100ms，连续循环

3. 按键控制：
   - Button B 长按 3 秒：静音/取消静音
   - 静音后所有 beep 和 alarm 都被忽略

4. 非阻塞实现，使用 millis()

请输出 BuzzerManager.h 和 BuzzerManager.cpp。
```

### 👤 你需要做的操作
1. 接上蜂鸣器（VCC→GPIO19, GND→GND）
2. 测试各种报警模式
3. 测试静音功能

### ✅ 验收标准
- [ ] 所有报警模式正常发声
- [ ] 静音功能正常
- [ ] 不影响主循环运行

---

## Task 024 — 按钮管理

### 🎯 任务目标
实现按钮的检测和消抖，支持单击和长按。

### 📖 背景说明
机械按钮按下时会产生抖动，导致多次触发。需要在软件层面做消抖处理。

### 🤖 给 Cloud Code 的提示词

```
请实现按钮管理模块。

要求：
1. ButtonManager 类：

```cpp
enum class ButtonEvent {
    NONE,
    CLICK,          // 单击（按下并释放）
    DOUBLE_CLICK,   // 双击
    LONG_PRESS,     // 长按（>1秒）
    VERY_LONG_PRESS // 超长按（>3秒）
};

class ButtonManager {
public:
    void begin(uint8_t pinA, uint8_t pinB);
    
    ButtonEvent getEventA();  // 获取按钮A的事件
    ButtonEvent getEventB();  // 获取按钮B的事件
    bool isPressedA();        // 按钮A是否正在按下
    bool isPressedB();
    
    void update();            // 在loop中调用（每10ms调用一次）
    
private:
    struct ButtonState {
        uint8_t pin;
        bool lastReading;
        bool pressed;
        unsigned long pressStartTime;
        unsigned long lastReleaseTime;
        bool waitingForDoubleClick;
        ButtonEvent pendingEvent;
    };
    
    ButtonState _btnA;
    ButtonState _btnB;
    
    ButtonEvent _detectEvent(ButtonState& btn);
    static const unsigned long DEBOUNCE_MS = 30;
    static const unsigned long LONG_PRESS_MS = 1000;
    static const unsigned long VERY_LONG_PRESS_MS = 3000;
    static const unsigned long DOUBLE_CLICK_MS = 400;
};
```

2. 按钮功能分配：
   Button A：
   - 单击 → 切换 OLED 显示页面
   - 双击 → 手动开关继电器（调试用）
   - 长按 → 进入/退出设置模式

   Button B：
   - 单击 → 恢复默认阈值
   - 长按3秒 → 静音/取消静音
   - 超长按5秒 → 恢复出厂设置

3. 消抖算法：
   - 每 ~10ms 读取一次
   - 连续 3 次读数一致才确认状态变化
   - 状态变化后锁定 30ms

4. 使用 INPUT_PULLUP（按下为 LOW）

请输出 ButtonManager.h 和 ButtonManager.cpp。
```

### 👤 你需要做的操作
1. 接上两个按钮（一端接 GPIO，另一端接 GND）
2. 编译烧录
3. 测试单击、双击、长按

### ✅ 验收标准
- [ ] 单击识别准确，无误触发
- [ ] 长按识别准确
- [ ] 双击识别准确

---

## Task 025 — 自动控制算法核心

### 🎯 任务目标
实现滞回控制算法，这是整个项目的大脑。

### 📖 背景说明
控制算法是项目核心。简单的"低于45%开、高于55%关"会导致临界点频繁开关。滞回控制+防抖+最小运行时间三重保护确保稳定。

### 🤖 给 Cloud Code 的提示词

```
请实现核心控制器 HumidifierController。

按照 docs/04_软件架构设计.md 第5.2节的滞回控制逻辑：

```cpp
class HumidifierController {
public:
    void begin();
    void loop();                     // 主循环（每2秒）
    
    SystemState getState();
    SensorData getCurrentData();
    
    // 参数调整
    void setHumidityLow(float val);
    void setHumidityHigh(float val);
    float getHumidityLow();
    float getHumidityHigh();
    
    // 手动控制（覆盖自动）
    void manualRelayOn();
    void manualRelayOff();
    void resumeAuto();
    bool isManualMode();
    
private:
    SensorManager _sensor;
    DisplayManager _display;
    RelayManager _relay;
    LedManager _led;
    BuzzerManager _buzzer;
    ButtonManager _buttons;
    
    SystemState _state;
    float _humidityLow;      // 45%
    float _humidityHigh;     // 55%
    bool _manualMode;
    
    // 防抖
    int _consecutiveLowReadings;
    int _consecutiveHighReadings;
    
    void _executeControlLogic();
    void _updateOutputs();
    void _processButtons();
};
```

控制逻辑实现：
1. 读取传感器数据
2. 如果数据无效 → 保持当前状态，错误处理
3. 湿度 < _humidityLow：
   - _consecutiveLowReadings++
   - 当 >= RELAY_DEBOUNCE_COUNT → 开启继电器
4. 湿度 > _humidityHigh：
   - _consecutiveHighReadings++
   - 当 >= RELAY_DEBOUNCE_COUNT → 关闭继电器
5. 在滞回区内：
   - 重置两个计数器
   - 保持当前继电器状态

状态管理：
- 继电器ON → 状态=HEATING, LED蓝色
- 继电器OFF → 状态=RUNNING, LED绿色
- 传感器故障 → 状态=SENSOR_ERROR, LED红色
- 手动模式 → 在OLED上显示"MANUAL"

请输出 HumidifierController.h 和 HumidifierController.cpp。
```

### 👤 你需要做的操作
1. 替换文件，编译烧录
2. 观察串口：湿度低于45%时继电器是否开启
3. 对着传感器哈气，观察湿度升高后是否关闭

### ✅ 验收标准
- [ ] 湿度 < 45% 时继电器开启
- [ ] 湿度 > 55% 时继电器关闭
- [ ] 45%~55% 之间保持当前状态
- [ ] 防抖功能生效（不是立即响应）

---

## Task 026 — 主程序集成

### 🎯 任务目标
将所有模块集成到 main.cpp，完成 V1.0 MVP 的核心功能。

### 📖 背景说明
这是最关键的一步：把手、脚、眼、脑拼成一个完整的人。所有模块在 main.cpp 中协调工作。

### 🤖 给 Cloud Code 的提示词

```
请编写 V1.0 最终版的 main.cpp，集成所有模块。

集成清单：
- SensorManager（传感器）
- DisplayManager（OLED显示）
- RelayManager（继电器，在 ActuatorManager 中）
- LedManager（LED指示，在 ActuatorManager 中）
- BuzzerManager（蜂鸣器）
- ButtonManager（按钮）
- HumidifierController（核心控制器）
- Logger（日志系统）

main.cpp 结构：

```cpp
#include "config.h"
#include "Logger.h"
#include "HumidifierController.h"

HumidifierController controller;

void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    delay(1000);  // 串口稳定
    
    Logger::printBanner();
    LOG_INFO("SYSTEM", "Starting initialization...");
    
    controller.begin();
    
    LOG_INFO("SYSTEM", "Initialization complete");
    LOG_INFO("SYSTEM", "Entering main loop");
}

void loop() {
    controller.loop();
    // controller.loop() 内部处理：
    // 1. 读取传感器
    // 2. 执行控制逻辑
    // 3. 更新显示
    // 4. 更新LED
    // 5. 处理按钮
    // 6. 检查报警
    // 7. 等待2秒（非阻塞）
}
```

controller.begin() 初始化流程：
1. LOG "Initializing I2C bus..."
2. I2C 初始化
3. LOG "Initializing Sensor..."
4. Sensor.begin()
5. LOG "Initializing Display..."
6. Display.begin() + 显示启动画面
7. LOG "Initializing Relay..."
8. Relay.begin() → 默认关闭
9. LOG "Initializing LED..."
10. LED 自检（红绿蓝各亮一下）
11. LOG "Initializing Buzzer..."
12. 短促提示音
13. LOG "Initializing Buttons..."
14. 显示 "System Ready" 2秒

请输出完整的 main.cpp。
```

### 👤 你需要做的操作
1. 确保所有模块文件都已放到 src/ 目录
2. 编译烧录
3. 观察整个系统的运行

### ✅ 验收标准
- [ ] 启动流程完整，每个步骤有日志
- [ ] 主循环正常工作
- [ ] 所有外设协调运行

---

## Task 027 — 异常场景测试

### 🎯 任务目标
模拟各种异常场景，验证系统鲁棒性。

### 📖 背景说明
真正的产品必须能处理各种异常情况。传感器断开、WiFi断连、断电重启等场景都需要覆盖。

### 🤖 给 Cloud Code 的提示词

```
请编写一个异常测试程序。

文件：firmware/test/test_exceptions.cpp

测试场景：
1. 传感器断开测试
   - 运行中拔掉 AHT20
   - 预期：LED变红色，蜂鸣器报警，OLED显示错误
   - 预期：继电器自动关闭（安全优先）
   - 5秒后插回传感器
   - 预期：自动恢复，LED变回绿色

2. 传感器数据异常测试
   - 模拟返回极端值
   - 预期：数据被验证函数拦截

3. 继电器保护测试
   - 连续快速调用 turnOn()/turnOff()
   - 预期：最小运行时间保护生效

4. 看门狗测试
   - 在代码中故意制造死循环
   - 预期：看门狗自动重启系统

5. 断电重启测试
   - 拔掉 USB 电源
   - 重新上电
   - 预期：系统正常启动，继电器默认关闭

6. 72小时连续运行记录
   - 输出运行统计
   - 记录任何异常事件

测试输出格式：
====================
Exception Tests
====================
[INFO] Test 1: Sensor disconnect - disconnect sensor NOW (5 seconds)
...等待...
[PASS] Test 1: Sensor error detected, LED red, relay off
[INFO] Test 1: Reconnect sensor NOW
...等待...
[PASS] Test 1: Sensor recovered, system normal
...

请输出 test_exceptions.cpp。
```

### 👤 你需要做的操作
1. 按提示逐一执行测试
2. 记录测试结果
3. 如有失败，报告给 Cloud Code 修复

### ✅ 验收标准
- [ ] 传感器断开 → 安全响应
- [ ] 传感器恢复 → 自动恢复
- [ ] 断电重启正常
- [ ] 无内存泄漏

---

## Task 028 — V1.0 MVP 完整功能验证

### 🎯 任务目标
对 V1.0 所有功能进行最终验证。

### 📖 背景说明
V1.0 MVP 的目标是：ESP32 成功启动 + AHT20 正常读取 + OLED 实时显示 + 继电器自动控制。确认这 4 个核心功能全部正常后，V1.0 才算完成。

### 🤖 给 Cloud Code 的提示词

```
请生成一份 V1.0 MVP 的功能验证清单。

文档：docs/V1.0_MVP功能验证清单.md

包含：
1. 全部功能项（从 F001 到 F011）的验收步骤
2. 每个功能的通过标准
3. 测试数据记录表格
4. 问题追踪表

验证项：
- [ ] ESP32 启动正常
- [ ] 串口输出日志格式正确
- [ ] AHT20 温湿度读取正常
- [ ] OLED 实时显示数据
- [ ] 湿度<45% 继电器ON
- [ ] 湿度>55% 继电器OFF
- [ ] 45%-55% 维持状态
- [ ] LED 颜色正确
- [ ] 蜂鸣器报警正常
- [ ] 按钮功能正常
- [ ] 传感器故障安全响应
- [ ] 连续运行 72 小时无异常

请输出完整文档。
```

### 👤 你需要做的操作
1. 逐项验证
2. 在清单上打勾
3. 记录任何发现的问题

### ✅ 验收标准
- [ ] 所有 P0 功能通过
- [ ] 72 小时稳定性测试无异常

---

> **Phase 4 完成！V1.0 MVP 全部功能正常工作。设备能自动控制加湿器，保持湿度在 45%-55%。**

---

# Phase 5：WiFi 联网 + Web 服务器 (V2.0)

---

## Task 029 — WiFiManager 实现

*（V2.0 阶段开发，此处列出任务结构）*

### 🎯 任务目标
实现 WiFi 连接管理，支持断线自动重连。

### 关键要求
- WiFi 连接/断开/重连
- SmartConfig 配网
- WiFi 信号强度读取
- 连接状态 LED 指示

---

## Task 030-032 — Web 服务器 + API

*（V2.0，暂略详细内容）*

---

## Task 033-036 — 网页仪表盘 + 历史数据

*（V2.0，暂略详细内容）*

---

# Phase 6：OTA + 优化 (V2.0)

---

## Task 037-041 — OTA 升级 + 代码优化

*（V2.0，暂略详细内容）*

---

# Phase 7：测试与发布

---

## Task 042 — 完整功能回归测试

### 🎯 任务目标
运行全部测试用例，确保所有功能正常。

---

## Task 043 — 文档完善

### 🎯 任务目标
更新 README、CHANGELOG，确保文档与代码一致。

---

## Task 044 — 代码审查

### 🎯 任务目标
检查代码质量：注释完整性、命名规范、无重复代码。

---

## Task 045 — V1.0 正式发布

### 🎯 任务目标
打 Git Tag V1.0.0，归档固件，发布到 GitHub。

---

## 📊 任务进度追踪

| Phase | 任务数 | 完成 | 进度 |
|-------|--------|------|------|
| Phase 1 | 8 | 0 | 0% |
| Phase 2 | 6 | 0 | 0% |
| Phase 3 | 6 | 0 | 0% |
| Phase 4 | 8 | 0 | 0% |
| Phase 5 | 8 | 0 | 0% |
| Phase 6 | 5 | 0 | 0% |
| Phase 7 | 4 | 0 | 0% |
| **总计** | **45** | **0** | **0%** |

---

> **使用说明**：每个 Task 中的"给 Cloud Code 的提示词"部分可以**直接复制**给 Claude Code / ChatGPT 等 AI 编程助手执行。每个 Task 完成后，在验收标准上打勾，然后进入下一个 Task。

> **重要原则**：一次只做一个 Task。不要提前写后面的代码。每完成一个 Task 就编译验证。
