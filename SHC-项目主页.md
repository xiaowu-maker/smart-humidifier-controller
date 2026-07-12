---
name: shc-project-home
description: Smart Humidifier Controller 项目总览主页
metadata:
  type: project
  status: v1.0-dev
  tech: [ESP32, C++, Arduino, PlatformIO, I2C, Embedded]
  started: 2026-07-11
  hardware_status: 待到货
  firmware_status: 编译通过
tags:
  - 嵌入式
  - ESP32
  - I2C
  - 智能硬件
  - 项目集
---

# 🌡️ Smart Humidifier Controller (SHC)

> **智能湿度控制器** — 从零打造的完整嵌入式 IoT 项目
>
> 适合面试展示：涵盖 需求分析 → 架构设计 → 模块开发 → 测试体系 全流程

---

## 📊 项目概览

| 维度 | 详情 |
|:--|:--|
| 项目类型 | 嵌入式 IoT 智能设备 |
| 主控芯片 | ESP32 DevKit V1 (Xtensa LX6 双核 240MHz) |
| 开发框架 | PlatformIO + Arduino Framework |
| 编程语言 | C++（嵌入式子集，面向对象设计） |
| 当前版本 | V1.0 MVP — 本地自动湿度控制 |
| 固件状态 | ✅ 全部 7 个模块编译通过 |
| 硬件状态 | 📦 等待快递到货 |
| BOM 成本 | ¥73 ~ ¥86 |

---

## 🗂️ 项目文档地图

### 核心入口

| 笔记 | 说明 |
|:--|:--|
| [[SHC-开发日志]] | 📝 每日开发记录（按时间线持续更新） |
| [[SHC-固件架构详解]] | 🏗️ 四层架构 + 7大模块设计（面试重点） |
| [[SHC-代码模块索引]] | 📦 每个 .h/.cpp 文件的功能说明 |
| [[SHC-编译与Bug记录]] | 🔧 编译统计 + 4个Bug的排查与修复过程 |

### 完整设计文档（SmartHumidifier/docs/）

| 文档 | 说明 |
|:--|:--|
| [[SmartHumidifier/docs/02_产品需求说明书(PRD)\|PRD 产品需求说明书]] | 功能需求、非功能需求、验收标准 |
| [[SmartHumidifier/docs/04_软件架构设计\|软件架构设计]] | 分层架构、状态机、数据流 |
| [[SmartHumidifier/docs/05_GPIO分配说明\|GPIO 分配说明]] | 11个引脚分配方案与约束分析 |
| [[SmartHumidifier/docs/03_硬件设计手册\|硬件设计手册]] | 硬件选型、接线图、电源设计 |
| [[SmartHumidifier/docs/07_CloudCode开发任务书\|CloudCode 开发任务书]] | 45个详细Task（可直接复制给AI） |
| [[SmartHumidifier/docs/08_测试手册\|测试手册]] | 58个测试用例 |
| [[SmartHumidifier/docs/09_Bug排查手册\|Bug 排查手册]] | 26个Bug条目+诊断工具 |
| [[SmartHumidifier/docs/10_版本规划\|版本规划]] | V1.0→V5.0 路线图 |
| [[SmartHumidifier/docs/01_项目开发计划(Roadmap)\|项目开发计划 Roadmap]] | Phase 0~8 完整路线 |
| [[SmartHumidifier/docs/06_通信协议\|通信协议]] | I2C、串口、Web API、MQTT |

---

## 🏗️ 系统架构一览

```
┌──────────────────────────────────────────────┐
│              主程序 main.cpp                   │  ← 主循环 + 控制逻辑
│         (滞回控制: <45%开 >55%关)              │
├──────────────────────────────────────────────┤
│  ┌──────────┐ ┌──────────┐ ┌──────────────┐  │
│  │ Sensor   │ │ Display  │ │  Actuator    │  │  ← 硬件抽象层
│  │ Manager  │ │ Manager  │ │  Manager     │  │
│  │ (AHT20)  │ │ (OLED)   │ │(Relay+LED+   │  │
│  │          │ │          │ │ Buzzer)      │  │
│  └────┬─────┘ └────┬─────┘ └──────┬───────┘  │
├───────┴────────────┴───────────────┴──────────┤
│  ┌──────────┐ ┌──────────┐ ┌──────────────┐  │
│  │ Logger   │ │ Serial   │ │  Button      │  │  ← 工具层
│  │ (4级日志) │ │ Command  │ │  Manager     │  │
│  │          │ │ (7命令)  │ │ (单击/双击/   │  │
│  │          │ │          │ │  长按/超长按) │  │
│  └──────────┘ └──────────┘ └──────────────┘  │
├──────────────────────────────────────────────┤
│          I2C Bus / GPIO / UART                │  ← 驱动层
│          ESP32 Arduino Core                   │
└──────────────────────────────────────────────┘
```

---

## 📈 版本路线

| 版本 | 代号 | 核心功能 | 状态 |
|:--|:--|:--|:--:|
| **V1.0** | Foundation | 本地自动控制（温湿度+OLED+继电器） | ✅ 代码完成 |
| V2.0 | Connected | WiFi + Web 远程控制 + 历史数据 | 📅 规划中 |
| V3.0 | Intelligent | AI 趋势分析 + MQTT + Home Assistant | 💡 构想 |
| V4.0 | Environmental | 多传感器（PM2.5/CO₂/VOC） | 💡 构想 |
| V5.0 | HomeHub | 家庭环境控制中心 + 开放API | 💡 构想 |

---

## 📊 V1.0 固件编译统计

```
编译时间 : 2026-07-11
RAM 使用 :  7.0%  (22KB  / 320KB)  ← 剩余 ~300KB
Flash使用: 21.5%  (280KB / 1.3MB)  ← 剩余 ~1MB
固件模块 :  7 个
串口命令 :  7 个
代码行数 : ~1,600 行
```

---

## 🎯 面试展示要点

如果你要向面试官展示这个项目，重点讲以下内容：

1. **需求驱动**：不是"我想做个东西"，而是"夏季空调导致湿度低 → 影响健康 → 需要自动控制"
2. **架构设计**：4层分层架构（驱动→硬件抽象→工具→应用），每个模块职责单一
3. **核心算法**：滞回控制算法（45%-55% 死区防止继电器频繁通断）
4. **容错设计**：传感器故障时优雅降级（使用缓存值 + 告警），不崩溃
5. **工程化**：完整的开发文档（PRD → 架构 → 测试 → Bug手册），不是"撸完代码就完"
6. **编译通过但未烧录**：硬件未到货，代码编写已完成（可以诚实说明）

详见 [[SHC-面试准备手册]]

---

## 👤 团队分工

| 角色 | 人员 | 工具 |
|:--|:--|:--|
| 项目总监 + 硬件搭建 | Kathryn | VS Code + 烙铁 |
| 系统架构设计 | ChatGPT | o1 模型 |
| 代码实现 | Claude Code | DeepSeek V4 Pro |
| 测试验证 | Kathryn（待硬件到货） | 串口监控 + 万用表 |

---

## 🔗 外部链接

- GitHub 仓库：待硬件验证后发布
- [[SmartHumidifier/README|项目 README]]
- [[SmartHumidifier/CHANGELOG|更新日志]]

---

> **最后更新**：2026-07-11 | **下次更新**：硬件到货后添加烧录与实测记录
