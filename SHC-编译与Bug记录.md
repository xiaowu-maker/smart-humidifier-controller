---
name: shc-build-bug-log
description: SHC 编译统计与 Bug 排查记录
metadata:
  type: project-reference
  project: SHC-项目主页
tags:
  - 编译
  - Bug
  - 调试
  - 排错
---

# 🔧 SHC 编译与 Bug 记录

> 记录每次编译统计数据、遇到的 Bug、排查过程和修复方案。
> 作为技术积累，面试时也可以展示调试能力。

---

## 📊 编译统计（2026-07-11）

### 最终编译结果

```
环境      : PlatformIO + ESP32 Arduino Core
平台      : espressif32 @ 6.9.0
框架      : Arduino
板型      : ESP32 DevKit V1 (esp32dev)
优化级别   : -Os (Size)

─────────────────────────────────────
RAM       :  22,000 / 327,680  (7.0%)
Flash     : 280,000 / 1,310,720 (21.5%)
─────────────────────────────────────

依赖库:
  Adafruit AHTX0    @ 2.0.5
  Adafruit SSD1306  @ 2.5.7
  Adafruit GFX      @ 1.11.9
  Adafruit BusIO    @ 1.14.5
  ArduinoJson       @ 7.0.4
  Wire              (内置)
```

### 分模块编译记录

| 顺序 | 模块 | 编译结果 | 备注 |
|:--:|:--|:--:|:--|
| 1 | platformio.ini + config.h | ✅ | 工程骨架 |
| 2 | Logger | ✅ | 基础日志输出 |
| 3 | SerialCommand | ✅ | 命令注册/解析 |
| 4 | SensorManager | ✅ | AHT20 驱动 |
| 5 | DisplayManager | ✅ | OLED 显示 |
| 6 | ActuatorManager | ✅ | Relay + LED + Buzzer |
| 7 | ButtonManager | ✅ | 按钮事件 |
| 8 | main.cpp 集成 | ✅ | 全部串联 |

---

## 🐛 Bug 记录

### Bug #1：`'Wire' was not declared in this scope`

| 项目 | 详情 |
|:--|:--|
| **发现时间** | 2026-07-11 |
| **触发模块** | `main.cpp` |
| **严重程度** | 🔴 编译错误（阻断） |

**错误输出**：
```
src/main.cpp: In function 'void setup()':
src/main.cpp:177:5: error: 'Wire' was not declared in this scope
   Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
   ^~~~
```

**根因分析**：

`main.cpp` 使用了 `Wire.begin()` 初始化 I2C 总线，但没有 `#include <Wire.h>`。虽然 `SensorManager` 和 `DisplayManager` 内部已经包含了 Wire.h，但 C++ 的 `#include` 是**文件级别**的——`main.cpp` 不包含 Wire.h，它就看不到 `Wire` 对象。

```
main.cpp
  └─ 没有 #include <Wire.h>
     └─ 编译器不知道 Wire 是什么
        └─ 报错！

SensorManager.h
  └─ 有 #include <Wire.h>
     └─ SensorManager.cpp 可以用 Wire ✅
```

**修复方案**：

在 `main.cpp` 头部添加：
```cpp
#include <Wire.h>
```

**学到的教训**：
- 每个 .cpp 文件是独立编译单元，需要的头文件必须在**自己的文件里**包含
- 不要依赖"间接包含"——A 包含了 B，不代表你就可以在 C 里直接用 B
- Arduino 框架中 `Wire` 对象定义在 `Wire.h` 中

---

### Bug #2：三元运算符类型不匹配

| 项目 | 详情 |
|:--|:--|
| **发现时间** | 2026-07-11 |
| **触发模块** | `main.cpp` cmdStatus() |
| **严重程度** | 🔴 编译错误（阻断） |

**错误输出**：
```
error: operands to ?: have different types
  'const __FlashStringHelper*' and 'const char*'
```

**原始代码**：
```cpp
Serial.println(sensor.isOK() ? F("OK") : sensor.getLastError());
```

**根因分析**：

这是 C++ 类型系统的经典陷阱。三元运算符 `a ? b : c` 要求 `b` 和 `c` **类型完全一致**：

- `F("OK")` 的返回类型是 `const __FlashStringHelper*`（字符串存在 Flash 中，ESP32 特性）
- `sensor.getLastError()` 的返回类型是 `const char*`（字符串在 RAM 中）
- 这两种指针类型不兼容！

`F()` 宏是 Arduino 的内存优化技巧——用 `F("string")` 包装的字符串字面量不会被复制到 RAM，而是直接从 Flash 读取，节省宝贵的 SRAM。但它返回的是特殊类型。

**修复方案**：

拆成 if/else：
```cpp
if (sensor.isOK()) {
    Serial.println(F("OK"));
} else {
    Serial.print(F("ERROR - "));
    Serial.println(sensor.getLastError());
}
```

**学到的教训**：
- C++ 的三元运算符比 C 更严格，要求两臂类型完全匹配
- `F()` 宏返回的不是普通的 `const char*`
- 遇到类型不匹配时，if/else 比强行用三元更清晰

---

### Bug #3：未使用变量警告

| 项目 | 详情 |
|:--|:--|
| **发现时间** | 2026-07-11 |
| **触发模块** | `DisplayManager.cpp` _drawSystemPage() |
| **严重程度** | 🟡 编译警告（不阻断） |

**警告输出**：
```
warning: unused variable 's' [-Wunused-variable]
   int s = uptime % 60;
       ^
```

**原始代码**：
```cpp
void DisplayManager::_drawSystemPage() {
    unsigned long uptime = (millis() - _bootTime) / 1000;
    unsigned long h = uptime / 3600;
    unsigned long m = (uptime % 3600) / 60;
    int s = uptime % 60;      // ← 声明了但从未使用！
    // 后面只用了 h 和 m，没用 s
}
```

**根因分析**：

写代码时预设了要显示秒数，但后来决定 OLED 页面只显示"XXh XXm"就够了（秒数变化太快，显示无意义），忘记删除 `s` 变量的声明。

**修复方案**：

删除 `int s = uptime % 60;` 这一行。

**学到的教训**：
- PlatformIO 默认开启了 `-Wunused-variable` 警告
- 写完代码后检查每个变量的使用情况
- 有警告就修，不要积累。10 个警告里可能藏着 1 个真正的 Bug

---

### Bug #4：`LedColor::WHITE` 与 SSD1306 宏冲突（⭐ 最有价值的 Bug）

| 项目 | 详情 |
|:--|:--|
| **发现时间** | 2026-07-11 |
| **触发模块** | `ActuatorManager.h` / `ActuatorManager.cpp` |
| **严重程度** | 🔴 编译错误（阻断）+ 经典 C++ 陷阱 |

**错误输出**：
```
error: expected '}' at end of enum definition
error: 'LedColor' does not name a type
```

**原始代码**：
```cpp
enum class LedColor {
    OFF,
    GREEN,
    BLUE,
    RED,
    YELLOW,
    WHITE    // ← 这个名字与 Adafruit_SSD1306.h 的宏冲突了！
};
```

**根因分析**：

这是 **C++ 预处理器宏的经典问题**。线索链：

1. `Adafruit_SSD1306.h` 中定义了：
```cpp
#define WHITE SSD1306_WHITE
```

2. 预处理器在编译前**全局**展开所有宏，不关心命名空间

3. 当 `LedColor::WHITE` 经过预处理器后，变成了：
```cpp
enum class LedColor {
    OFF,
    GREEN,
    BLUE,
    RED,
    YELLOW,
    1          // ← WHITE 被替换成了 SSD1306_WHITE（值为 1）
};
```

4. 编译器看到的是一个整数 `1` 作为枚举成员名，直接语法错误！

**为什么 `enum class` 的命名空间保护不了？**

因为 C/C++ 的 `#define` 宏是在**预处理阶段**处理的，发生在编译器理解 C++ 语法之前。预处理器不知道什么是 `enum class`，不知道什么是命名空间——它只是机械地做文本替换。

```
编译流程：
源代码 → [预处理器 (宏展开)] → [编译器 (语法分析)] → [链接器]
                ↑
          #define WHITE 在这里替换所有 "WHITE" 文本
          不管它在哪个命名空间、哪个类、哪个枚举里！
```

**修复方案**：

将枚举值改名，避免与任何库的宏冲突：
```cpp
enum class LedColor {
    OFF,
    GREEN,
    BLUE,
    RED,
    YELLOW,
    COLOR_WHITE   // ← 加前缀，避免冲突
};
```

同步修改所有引用处（`ActuatorManager.cpp`、`main.cpp`）：
```cpp
led.set(LedColor::COLOR_WHITE, LedPattern::SLOW_BLINK);
```

**学到的教训**：
- **宏是全局的，不尊重任何作用域**。这是 C/C++ 最重要的教训之一
- 给枚举值加前缀是个好习惯（如 `COLOR_WHITE`、`STATE_IDLE`）
- 当出现莫名其妙的编译错误时，优先怀疑宏冲突——用 `gcc -E` 查看预处理结果
- Adafruit 库的 `WHITE` / `BLACK` 宏是已知的"污染源"，很多开发者踩过这个坑
- 这也解释了为什么很多库用 `kWhite`、`COLOR_WHITE`、`Color::White` 等变体

---

## 📈 Bug 统计

| 类型 | 数量 | 解决 |
|:--|:--:|:--:|
| 编译错误（阻断） | 3 | ✅ |
| 编译警告 | 1 | ✅ |
| 运行时 Bug | 0 | ⏳ 硬件到货后验证 |
| **总计** | **4** | **全部修复** |

---

## 📂 相关笔记

- [[SHC-项目主页]] — 返回项目总览
- [[SHC-固件架构详解]] — 架构设计文档
- [[SHC-代码模块索引]] — 代码文件索引
- [[SHC-开发日志]] — 每日开发记录
