# Smart Humidifier Controller (SHC)

<p align="center">
  <b>🌡️ 智能湿度控制器 — 让房间湿度始终保持在 45%~55% 人体舒适范围</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-1.0.0-blue.svg" alt="version">
  <img src="https://img.shields.io/badge/platform-ESP32-green.svg" alt="platform">
  <img src="https://img.shields.io/badge/framework-Arduino-00979D.svg" alt="framework">
  <img src="https://img.shields.io/badge/license-MIT-yellow.svg" alt="license">
  <img src="https://img.shields.io/badge/build-PlatformIO-orange.svg" alt="build">
</p>

---

## 📖 项目简介

夏季长时间使用空调会导致室内空气湿度下降，引起咽喉干燥、咳嗽、睡眠质量下降等问题。

**Smart Humidifier Controller (SHC)** 是一套低成本、高可靠性的智能湿度管理系统。它能：

- ✅ **实时检测**室内温湿度（AHT20 传感器，每2秒刷新）
- ✅ **自动控制**加湿器开关（湿度 < 45% 开启，> 55% 关闭）
- ✅ **OLED 显示**实时数据（温度、湿度、系统状态）
- ✅ **LED 状态指示**（绿=正常，蓝=加湿中，红=异常）
- ✅ **蜂鸣器报警**（传感器故障、WiFi断开等）
- ✅ **滞回控制**防抖算法，避免频繁开关

> 🎯 **核心目标：让房间湿度始终保持在人体最舒适的范围（45%~55%）。**

---

## 🎬 快速开始

### 硬件要求

| 硬件 | 数量 | 约价 |
|------|:----:|-----:|
| ESP32 DevKit V1 | 1 | ¥25 |
| AHT20 温湿度传感器 | 1 | ¥8 |
| 0.96寸 OLED (SSD1306, I2C) | 1 | ¥12 |
| 单路继电器模块 | 1 | ¥5 |
| 三色 RGB LED (共阴极) | 1 | ¥2 |
| 有源蜂鸣器 | 1 | ¥3 |
| 按钮 (6×6mm) | 2 | ¥1 |
| 面包板 + 杜邦线 | 1套 | ¥15 |
| Micro USB 数据线 | 1 | ¥5 |
| 5V/2A USB 电源 | 1 | ¥10 |
| **合计** | | **约 ¥86** |

### 软件要求

- **VS Code** + PlatformIO 插件
- **ESP32 Arduino Core**（PlatformIO 自动安装）
- **USB 驱动**：CH340 或 CP2102（取决于 ESP32 开发板型号）

### 3 步跑起来

```bash
# 1. 克隆项目
git clone https://github.com/yourusername/SmartHumidifier.git
cd SmartHumidifier/firmware

# 2. 编译
pio run

# 3. 烧录（ESP32 通过 USB 连接到电脑）
pio run --target upload

# 4. 查看串口输出
pio device monitor --baud 115200
```

---

## 📁 项目结构

```
SmartHumidifier/
├── docs/                           # 📚 完整开发文档
│   ├── 01_项目开发计划(Roadmap).md
│   ├── 02_产品需求说明书(PRD).md
│   ├── 03_硬件设计手册.md
│   ├── 04_软件架构设计.md
│   ├── 05_GPIO分配说明.md
│   ├── 06_通信协议.md
│   ├── 07_CloudCode开发任务书.md    # 🤖 AI 编程任务书（45个Task）
│   ├── 08_测试手册.md
│   ├── 09_Bug排查手册.md
│   └── 10_版本规划.md
├── hardware/                       # 🔧 硬件资料
│   ├── schematic/                  # 原理图
│   └── bom.md                      # 物料清单
├── firmware/                       # 💾 固件代码
│   ├── platformio.ini
│   ├── src/
│   │   ├── main.cpp                # 程序入口
│   │   ├── config.h                # 全局配置
│   │   ├── Logger.h/cpp            # 日志系统
│   │   ├── SensorManager.h/cpp     # 传感器管理
│   │   ├── DisplayManager.h/cpp    # OLED显示管理
│   │   ├── ActuatorManager.h/cpp   # 执行器管理(Relay+LED+Buzzer)
│   │   ├── ButtonManager.h/cpp     # 按钮管理
│   │   ├── HumidifierController.h/cpp  # 核心控制器
│   │   ├── WiFiManager.h/cpp       # WiFi管理 (V2.0)
│   │   └── WebServerManager.h/cpp  # Web服务 (V2.0)
│   ├── include/
│   ├── data/                       # Web页面文件(SPIFFS)
│   └── test/                       # 单元测试
├── images/                         # 📷 项目图片
├── tests/                          # 🧪 集成测试
├── README.md
├── CHANGELOG.md
└── LICENSE
```

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────┐
│              HumidifierController             │  ← 核心大脑
│         (决策引擎: 读取→判断→控制)              │
├─────────────────────────────────────────────┤
│  ┌──────────┐  ┌──────────┐  ┌────────────┐ │
│  │ Sensor   │  │ Display  │  │  Actuator   │ │  ← HAL层
│  │ Manager  │  │ Manager  │  │  Manager    │ │
│  │ (AHT20)  │  │ (OLED)   │  │ (Relay/LED) │ │
│  └────┬─────┘  └────┬─────┘  └──────┬─────┘ │
├───────┴─────────────┴────────────────┴───────┤
│              I2C Bus / GPIO                   │  ← 驱动层
│          ESP32 Arduino Core                   │
└─────────────────────────────────────────────┘
```

### 控制算法（滞回控制）

```
湿度 < 45%  ──→ 继电器 ON  ──→ 加湿器工作 ──→ LED 蓝色
湿度 > 55%  ──→ 继电器 OFF ──→ 加湿器停止 ──→ LED 绿色
45%~55%     ──→ 保持当前状态（滞回区，避免频繁开关）
```

三重保护：
1. **滞回区间**：45%~55% 之间不改变状态
2. **采样防抖**：连续 3 次（6秒）超阈值才动作
3. **最小运行时间**：状态变化后至少维持 60 秒

---

## 🔌 引脚接线

| ESP32 引脚 | 连接设备 | 说明 |
|:----------:|----------|------|
| GPIO21 | AHT20 SDA + OLED SDA | I2C 数据线（共享） |
| GPIO22 | AHT20 SCL + OLED SCL | I2C 时钟线（共享） |
| GPIO4 | 继电器 IN | 控制加湿器通断 |
| GPIO16 | RGB LED R | 红色通道 |
| GPIO17 | RGB LED G | 绿色通道 |
| GPIO18 | RGB LED B | 蓝色通道 |
| GPIO19 | 蜂鸣器 | 报警输出 |
| GPIO13 | 按钮 A | 模式切换（内部上拉） |
| GPIO14 | 按钮 B | 恢复默认（内部上拉） |
| 3.3V | AHT20 VCC + OLED VCC | 传感器和屏幕供电 |
| 5V | 继电器 VCC | 继电器供电 |
| GND | 所有模块 GND | 共地 |

> 📖 详细接线图见 `docs/03_硬件设计手册.md`

---

## 📊 版本路线

| 版本 | 代号 | 核心功能 | 状态 |
|------|------|----------|:----:|
| **V1.0** | Foundation | 本地自动控制（温湿度采集+OLED+继电器） | 🔄 开发中 |
| V2.0 | Connected | WiFi联网 + Web远程控制 + 历史数据 | 📅 规划中 |
| V3.0 | Intelligent | AI趋势分析 + MQTT + Home Assistant | 💡 构想中 |
| V4.0 | Environmental | 多传感器（PM2.5/CO₂/VOC）+ 场景联动 | 💡 构想中 |
| V5.0 | HomeHub | 家庭环境控制中心 + 开放API生态 | 💡 构想中 |

---

## 🧪 测试

```bash
# 运行传感器测试
pio test -e test_sensor

# 运行显示测试
pio test -e test_display
```

完整测试文档见 `docs/08_测试手册.md`

---

## 📚 开发文档

本项目是一套**完整的 AI 辅助开发教程**。每份文档都按专业软件工程标准编写：

| 文档 | 说明 |
|------|------|
| [项目开发计划](docs/01_项目开发计划(Roadmap).md) | 从 Phase 0 到 Phase 8 的完整路线图 |
| [产品需求说明书](docs/02_产品需求说明书(PRD).md) | 功能需求、非功能需求、验收标准 |
| [硬件设计手册](docs/03_硬件设计手册.md) | 硬件选型、接线图、电源设计、采购指南 |
| [软件架构设计](docs/04_软件架构设计.md) | 分层架构、模块设计、数据流、状态机 |
| [GPIO分配说明](docs/05_GPIO分配说明.md) | 引脚分配方案及约束分析 |
| [通信协议](docs/06_通信协议.md) | I2C、串口、Web API、MQTT 协议规范 |
| [Cloud Code 任务书](docs/07_CloudCode开发任务书.md) | **45个详细开发任务（可直接复制给AI）** |
| [测试手册](docs/08_测试手册.md) | 单元测试、集成测试、系统测试、异常测试 |
| [Bug 排查手册](docs/09_Bug排查手册.md) | 常见问题诊断与解决方案 |
| [版本规划](docs/10_版本规划.md) | V1.0→V5.0 版本路线与发布规范 |

---

## 🤝 贡献指南

本项目欢迎贡献！请遵循以下原则：

1. **模块化**：每个模块职责单一
2. **有注释**：所有函数和关键逻辑有中文注释
3. **有测试**：新功能附带测试用例
4. **先文档**：架构变更先更新文档，再改代码

### Commit 规范

```
<type>(<scope>): <subject>

类型：feat / fix / docs / refactor / test / chore
范围：sensor / display / relay / control / wifi / docs
```

---

## 📄 许可证

本项目采用 **MIT 许可证**。详见 [LICENSE](LICENSE)。

---

## 👤 作者

- **项目负责人**：Kathryn
- **系统架构设计**：ChatGPT
- **代码实现**：Claude Code
- **硬件搭建与测试**：Kathryn

---

## ⭐ 致谢

- [Adafruit](https://adafruit.com) — AHTX0 & SSD1306 驱动库
- [PlatformIO](https://platformio.org) — 嵌入式开发平台
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) — ESP32 Arduino 框架

---

<p align="center">
  <b>Made with ❤️ by Kathryn | 从零开始打造 AI 智能湿度控制器</b>
</p>
