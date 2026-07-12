# CHANGELOG

本文件记录 Smart Humidifier Controller (SHC) 所有重要变更。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

---

## [1.0.0] — 开发中

> **目标**：本地智能湿度自动控制（MVP）
> **预计完成**：硬件到货后 1-2 周

### ✅ 已完成

#### 文档体系（13 份）
- `01_项目开发计划(Roadmap).md` — Phase 0~8 完整路线图
- `02_产品需求说明书(PRD).md` — 功能需求/非功能需求/验收标准
- `03_硬件设计手册.md` — 硬件选型/接线图/电源设计/采购指南
- `04_软件架构设计.md` — 4层架构/7大模块/状态机/数据流
- `05_GPIO分配说明.md` — 11个GPIO分配方案+约束分析
- `06_通信协议.md` — I2C/串口/Web API/MQTT 协议
- `07_CloudCode开发任务书.md` — 45个详细Task
- `08_测试手册.md` — 58个测试用例
- `09_Bug排查手册.md` — 26个Bug条目+诊断工具
- `10_版本规划.md` — V1.0→V5.0 路线图+发布规范
- `README.md` — 项目主页
- `LICENSE` — MIT 开源协议
- `CHANGELOG.md` — 本文件

#### 固件模块（6 个模块，编译通过 ✅）
- `platformio.ini` — ESP32 + Arduino + 5个依赖库
- `config.h` — 全局配置（引脚/阈值/WiFi/日志级别）
- `Logger.h/cpp` — 4级日志系统（DEBUG/INFO/WARN/ERROR）
- `SerialCommand.h/cpp` — 串口命令解析器（7个命令：STATUS/SENSOR/RELAY/BUZZER/HELP/INFO/RESET）
- `SensorManager.h/cpp` — AHT20 传感器驱动（读取/验证/校准/异常处理）
- `DisplayManager.h/cpp` — OLED 显示（3页面/反白标题/页面指示器）
- `ActuatorManager.h/cpp` — 执行器（Relay+LED+Buzzer，非阻塞）
- `main.cpp` — 主程序入口，集成所有模块

#### 核心功能（代码已实现）
- ✅ 温湿度采集（AHT20，每 2 秒）
- ✅ 数据验证（范围检查 + 跳变过滤 + 连续错误检测）
- ✅ OLED 多页面显示（主页面/网络信息/系统信息）
- ✅ 滞回控制（<45% 开，>55% 关，45%-55% 保持）
- ✅ 继电器最短运行时间保护（60秒）
- ✅ LED 状态指示（绿=正常，蓝=加湿，红=异常）
- ✅ 蜂鸣器分级报警（传感器故障/WiFi断开/严重故障）
- ✅ 串口命令交互系统
- ✅ 手动/自动模式切换

### 🔲 待完成（阻塞：硬件未到货）

| 事项 | 阻塞原因 |
|------|----------|
| 烧录到 ESP32 验证 | ESP32 未到 |
| AHT20 接线 + I2C 扫描验证 | 传感器未到 |
| OLED 接线 + 显示效果验证 | OLED 未到 |
| 继电器 + LED + 蜂鸣器硬件验证 | 模块未到 |
| 按钮模块 (ButtonManager) | 按钮未到（V1.0 P1 可选） |
| 72 小时稳定性测试 | 需要完整硬件 |

### 📦 V1.0 固件编译统计

```
RAM:   6.6%  (21KB / 320KB)
Flash: 21.2% (277KB / 1.3MB)
编译时间: ~4 秒
```

---

## [2.0.0] — 规划中

> 代号：Connected | 预计：V1.0 稳定后启动

- WiFi 连接管理 + SmartConfig 配网
- Web 服务器 + JSON API
- 手机网页仪表盘
- 24 小时历史数据（SPIFFS）

## [3.0.0] — 构想中

> 代号：Intelligent

- MQTT + Home Assistant 集成
- AI 湿度趋势分析
- 自动学习用户习惯

---

> **当前状态**：V1.0 代码编写完成，等待硬件到货进行烧录和功能验证。
