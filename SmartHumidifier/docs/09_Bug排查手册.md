# 09 - Bug 排查手册

> **项目**: SmartHumidifier — ESP32 DevKit V1 智能湿度控制器  
> **文档版本**: 1.0  
> **最后更新**: 2026-07-11  
> **适用人群**: 初学者 / DIY 爱好者 / 嵌入式开发者

---

## 目录

1. [编译与烧录问题](#1-编译与烧录问题)
2. [硬件问题](#2-硬件问题)
3. [运行时问题](#3-运行时问题)
4. [WiFi 问题（V2.0）](#4-wifi-问题v20)
5. [诊断工具](#5-诊断工具)
6. [调试技巧](#6-调试技巧)
7. [附录：错误码速查](#7-附录错误码速查)

---

## 1. 编译与烧录问题

---

### B001：platformio.ini 配置错误导致编译失败

**症状描述**：
在 VS Code 中点击编译（Build）后，终端输出大量错误信息，常见的有 `Unknown build_flags`、`Unknown board ID`、`Error: Could not find the platform` 等。编译过程中途停止，生成 .bin 文件失败。

**可能原因**（按概率排列）：
1. `platformio.ini` 中 board 名称拼写错误或与实际使用的开发板不匹配。
2. `platform` 字段指定的平台（如 `espressif32`）版本号不存在或已废弃。
3. `build_flags` 中使用了不存在的编译器选项或宏定义格式错误。
4. 缩进不规范 — PlatformIO 的 INI 解析器对缩进敏感，多余的缩进会导致解析错误。
5. 同时指定了冲突的 `framework`（例如同时写 `arduino` 和 `espidf`）。

**诊断步骤**：
1. 检查 VS Code 终端最前方的错误行，确认错误类型（`platformio.ini` 相关错误一般在第 1~3 行就能看到）。
2. 打开 `platformio.ini`，逐行检查 board 名称：输入 `pio boards` 命令可以列出所有支持的 board，确认你使用的名称在列表中。
3. 检查 platform 版本号，例如 `platform = espressif32@5.2.0` — 可以尝试去掉版本号让 PlatformIO 自动获取最新版。
4. 检查所有 `build_flags` 是否以 `-D` 或 `-I` 开头，例如 `-DLED_BUILTIN=2` 是正确的，而 `LED_BUILTIN=2`（缺少 `-D`）是错误的。
5. 确认 INI 文件没有多余的缩进 — section 标题（如 `[env:esp32dev]`）必须顶格书写，键值对前面不能有空格。

**解决方案**：
- 将 board 修正为正确的名称，如 `board = esp32dev`（对应 ESP32 DevKit V1）。
- 升级或固定 platform 版本：`platform = espressif32`（不指定版本号则自动获取最新稳定版）。
- 修正 `build_flags` 格式：每个 flag 一行，行首加 `-D` 或 `-I`，示例：
  ```ini
  build_flags =
      -DLED_BUILTIN=2
      -DDEBUG_LEVEL=3
  ```
- 恢复所有 section 标题顶格，删除多余空格。

**参考代码 — platformio.ini 正确模板**：

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
board_build.flash_mode = dio

lib_deps =
    adafruit/Adafruit AHTX0@^2.0.5
    adafruit/Adafruit SSD1306@^2.5.7
    adafruit/Adafruit GFX Library@^1.11.9

build_flags =
    -DLED_BUILTIN=2
    -DCORE_DEBUG_LEVEL=1

board_build.partitions = default_16MB.csv
```

---

### B002：库依赖找不到（Library Not Found）

**症状描述**：
编译时终端报错 `fatal error: Adafruit_AHTX0.h: No such file or directory` 或 `Library "xxx" not found`。编译立即终止。

**可能原因**：
1. `platformio.ini` 的 `lib_deps` 中未声明该库。
2. 库名称格式错误 — PlatformIO 库注册名与头文件名不完全一致。
3. 网络原因导致 PlatformIO 无法从库管理器下载库。
4. 本地安装了多个版本的同一库，导致版本冲突。
5. 手动将库文件夹放入了 `lib/` 目录但缺少依赖的底层库。

**诊断步骤**：
1. 查看终端中报错的具体头文件名，例如 `Adafruit_AHTX0.h`。
2. 打开 `platformio.ini`，检查 `lib_deps` 列表中是否包含该库。
3. 在 VS Code 终端输入 `pio lib search "Adafruit AHTX0"` 确认库的正确注册名称。
4. 检查本地 `~/.platformio/packages/framework-arduinoespressif32/libraries/` 目录下是否已有该库。
5. 尝试注释掉 `lib_deps` 中其他库，仅保留出问题的库进行最小化编译测试。

**解决方案**：
- 在 `lib_deps` 中正确添加库，格式为 `作者/库名@版本号`，例如：
  ```ini
  lib_deps =
      adafruit/Adafruit AHTX0@^2.0.5
  ```
- 如果网络被墙，配置国内镜像源：在 `platformio.ini` 中添加：
  ```ini
  lib_storage = esp32
  lib_extra_storage =
      https://cdn.jsdelivr.net/gh/platformio/platformio-libraries@gh-pages
  ```
- 清除库缓存后重试：在终端执行 `pio lib cache clean && pio update`。
- 如果手动安装库，确保将所有依赖库一并放入 `lib/` 目录，或者统一通过 `lib_deps` 声明。

---

### B003：编译内存溢出（Flash/RAM 不足）

**症状描述**：
编译成功但最后出现 `.text section exceeds available space in flash`、`region `dram0_0_seg' overflowed` 或 `sketch too big` 等错误。固件体积超标，无法生成完整的二进制文件。

**可能原因**：
1. 启用了过多或过重的库（例如同时启用 Adafruit GFX + TFT_eSPI + LVGL）。
2. 分区表（partitions）配置过小，尤其是默认的 4MB 闪存分区不能满足程序需求。
3. 代码中包含大量静态字符串或全局缓冲区。
4. 启用了不必要的调试输出（`-DCORE_DEBUG_LEVEL=5` 会产生大量日志代码）。

**诊断步骤**：
1. 观察编译终端最后几行，查看 Flash 占用比例，例如：
   ```
   RAM:   [===       ]  35.0% (used 114628 bytes from 327680 bytes)
   Flash: [======    ]  59.8% (used 1252494 bytes from 2097152 bytes)
   ```
2. 运行 `pio size --board esp32dev` 查看各段详细用量。
3. 检查分区表：查看 `board_build.partitions` 指向的文件。
4. 检查代码中是否有大型的 `static const char` 数组或 bitmap 图片数据。

**解决方案**：
- 使用更大的分区表。在 `platformio.ini` 中指定：
  ```ini
  board_build.partitions = default_16MB.csv
  ```
  或使用自定义分区表，创建一个 `partitions.csv`：
  ```
  # Name,   Type, SubType, Offset,  Size,    Flags
  nvs,      data, nvs,     0x9000,  0x5000,
  otadata,  data, ota,     0xe000,  0x2000,
  app0,     app,  ota_0,   0x10000, 0x1E0000,
  app1,     app,  ota_1,   0x1F0000,0x1E0000,
  eeprom,   data, 0x99,    0x3D0000,0x1000,
  spiffs,   data, spiffs,  0x3D1000,0x2EF00,
  ```
- 减少不必要的库，例如如果只需 OLED 文字显示，可以不加载 Adafruit GFX 的字体文件。
- 降低调试级别：`-DCORE_DEBUG_LEVEL=1`（仅错误）或 `=0`（关闭）。
- 将大字符串放入 PROGMEM：
  ```cpp
  const char long_string[] PROGMEM = "这是一段很长的字符串...";
  ```

---

### B004：烧录连接失败（Failed to Connect）

**症状描述**：
点击 Upload（烧录）后，终端反复输出 `Connecting........_____....._____.....` 最终提示 `A fatal error occurred: Failed to connect to ESP32: Timed out waiting for packet header`。烧录无法进行。

**可能原因**：
1. ESP32 未进入下载模式（Bootloader 模式）。
2. 串口线不支持数据传输（仅充电线）。
3. USB 驱动未安装或串口芯片（CP2102 / CH340）驱动异常。
4. 错误的串口号（COM Port）。
5. 板载电源不稳定或 USB 线过长导致信号衰减。

**诊断步骤**：
1. 检查 Windows 设备管理器（设备管理器）— 展开「端口（COM 和 LPT）」查看是否有 `Silicon Labs CP210x USB to UART Bridge` 或类似条目，记录 COM 号。
2. 按住 ESP32 开发板上的 **BOOT（GPIO0）** 按钮不放，再短按 **EN（RST）** 按钮后松开 BOOT，让芯片进入下载模式。
3. 在终端输入 `pio device list` 查看所有串口设备。
4. 换一根支持数据传输的 USB 线（充电线通常缺少 D+/D- 数据线）。
5. 尝试降低烧录波特率：在 `platformio.ini` 添加 `upload_speed = 115200`（默认通常为 921600）。

**解决方案**：
- 手动进入下载模式：按住 BOOT → 按一下 EN → 松开 BOOT，然后立即点击 Upload。
- 在 `platformio.ini` 中启用 DTR/RTS 自动复位（部分 ESP32 板支持）：
  ```ini
  upload_flags =
      -D DTR_RESET_ENABLE
  ```
- 安装或更新串口驱动：
  - CP2102 驱动：https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
  - CH340 驱动：http://www.wch.cn/download/CH341SER_EXE.html
- 手动指定 COM 端口：
  ```ini
  upload_port = COM3
  ```
- 降低烧录速度以提高稳定性：
  ```ini
  upload_speed = 115200
  ```

---

### B005：串口不识别 / 找不到 COM 口

**症状描述**：
Windows 设备管理器中看不到任何 COM 端口，或者插入 ESP32 后无任何反应（无"叮咚"提示音）。PlatformIO 无法找到上传目标。

**可能原因**：
1. USB 驱动程序未安装。
2. USB 线是纯充电线（缺少数据线路）。
3. ESP32 开发板上的串口芯片损坏。
4. 电脑 USB 端口故障或供电不足。
5. 与其他 USB 设备冲突（少见）。

**诊断步骤**：
1. 将 ESP32 插入电脑不同 USB 口（建议使用主板背部 USB 口而非前置面板）。
2. 在设备管理器中查看「其他设备」下是否有黄色感叹号未知设备。
3. 更换已知能传输数据的 USB 线重新测试。
4. 在另一台电脑上插入测试，判断是否为电脑问题。
5. 检查 ESP32 板载 LED（通常有电源指示灯 PWR）是否亮起。

**解决方案**：
- 安装 CP2102 驱动（大多数 ESP32 DevKit V1 使用此芯片）：
  - 下载地址：https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
  - 安装后重启电脑。
- 如果是 CH340 芯片（某些低价 ESP32 开发板使用），安装 CH340 驱动：http://www.wch.cn/download/CH341SER_EXE.html
- 换用高质量 USB 数据线，确保线长不超过 2 米。
- 如果板载串口芯片损坏，可以使用外接 USB-TTL 模块连接 ESP32 的 TX/RX 引脚进行烧录。

---

### B006：串口监视器波特率不匹配

**症状描述**：
串口监视器（Serial Monitor）打开后输出乱码（一堆奇怪符号）或无任何输出。程序正常运行（可以通过 LED 等判断），但无法看到调试日志。

**可能原因**：
1. 代码中 `Serial.begin(115200)` 与 PlatformIO 的 `monitor_speed` 设置不一致。
2. 串口监视器的波特率设置与程序实际波特率不同。
3. ESP32 复位后串口初始化的延迟，导致在 Serial 初始化之前输出了乱码。

**诊断步骤**：
1. 确认代码中的波特率：搜索 `Serial.begin(` 看传入的参数值。
2. 检查 `platformio.ini` 中的 `monitor_speed` — 必须与此值一致。
3. 在 VS Code 中打开串口监视器（点击底部状态栏的插头图标），查看右上角波特率下拉框中的设置。
4. 程序启动时，ESP32 会通过串口输出启动日志（ROM bootloader 输出，波特率 115200），如果此部分乱码则是监视器设置问题。

**解决方案**：
- 在 `platformio.ini` 中设置正确的波特率：
  ```ini
  monitor_speed = 115200
  ```
- 在代码开头添加短延时，确保串口稳定后再输出：
  ```cpp
  void setup() {
      Serial.begin(115200);
      delay(100);  // 等待串口稳定
      Serial.println("System starting...");
  }
  ```
- 如果启动日志（ROM bootloader 输出）也是乱码，可以尝试切换监视器的行结束符设置为「Both NL & CR」。
- 在其他串口工具（如 Putty、Arduino IDE 串口监视器）中测试相同的波特率，排除 PlatformIO 的问题。

---

### B007：分区表错误导致启动崩溃

**症状描述**：
程序烧录成功，但 ESP32 不断重启，串口输出类似 `E (259) esp_image: image at 0x10000 has invalid magic byte`、`Brownout detector was triggered` 或 `ets Jun  8 2016 00:22:57` 后反复复位。

**可能原因**：
1. 分区表与实际闪存大小不匹配（例如 4MB 闪存用了 16MB 的分区表）。
2. 烧录了错误的引导程序（bootloader）。
3. 闪存模式设置错误（DIO / QIO / DOUT 不匹配）。
4. 闪存频率设置过高，超出硬件支持。

**诊断步骤**：
1. 查看复位循环中的串口输出，关注 `ets_main.c` 或 `esp_image` 相关错误。
2. 确认开发板的闪存容量：`esptool.py flash_id` 命令可以读取闪存信息。
3. 检查 `platformio.ini` 中 `board_build.flash_mode` 和 `board_build.partitions` 的设置。
4. 尝试擦除整片闪存后重新烧录：在终端执行 `pio run --target erase`。

**解决方案**：
- 擦除整片闪存后重新烧录：
  ```
  pio run --target erase
  pio run --target upload
  ```
- 正确配置闪存模式。ESP32 DevKit V1 通常使用 DIO 模式：
  ```ini
  board_build.flash_mode = dio
  ```
- 选择与硬件匹配的分区表：
  ```ini
  board_build.partitions = default.csv       # 4MB 闪存
  ; board_build.partitions = default_16MB.csv  # 16MB 闪存
  ```
- 如果使用自定义分区表，确保偏移量和大小正确对齐（每个分区起始地址必须是 0x1000 的倍数）。

---

### B008：PlatformIO 构建缓存导致异常

**症状描述**：
修改了代码或库文件后重新编译，但终端提示 `Nothing to build` 或编译结果未包含最近修改。手动删除了某个库，编译仍然成功（但实际上应该失败）。

**可能原因**：
1. PlatformIO 的构建缓存（build cache）未及时刷新。
2. `lib/` 目录下的本地库修改后，PlatformIO 未检测到变更。
3. 全局缓存 (`~/.platformio/.cache`) 中的旧版本库被优先使用。

**诊断步骤**：
1. 观察编译日志是否有 `Using cached` 字样。
2. 检查 `lib/` 目录下的文件和 `platformio.ini` 的 `lib_deps` 是否有重复声明。
3. 尝试执行完整的 clean 重建。

**解决方案**：
- 执行完整清理后重新编译：
  ```
  pio run --target clean
  pio run
  ```
- 如果问题依旧，手动清除 PlatformIO 缓存目录：
  ```
  rm -rf ~/.platformio/.cache
  ```
- 在 VS Code 中点击 PlatformIO 侧边栏的 `Rebuild` 按钮（带刷新的三角形图标）强制全量重编。
- 在 `platformio.ini` 中临时禁用缓存（不推荐长期使用）：
  ```ini
  build_cache =
  ```

---

## 2. 硬件问题

---

### B009：I2C 设备扫描不到

**症状描述**：
程序运行时 OLED 无显示、AHT20 数据始终为 0 或 NaN（无效）。使用 I2C 扫描代码（见第 5 章诊断工具）发现无设备响应，返回 `No I2C devices found`。

**可能原因**：
1. I2C 总线接线错误 — SDA 与 SCL 接反、或接到了错误的引脚。
2. VCC 供电不足 — 两个 I2C 设备同时连接而 ESP32 的 3.3V 输出电流不够。
3. I2C 上拉电阻缺失 — 虽然 ESP32 内部有弱上拉，但长线或总线电容较大时仍需外部 4.7kΩ 上拉电阻。
4. I2C 地址冲突 — 虽然 AHT20（0x38）和 SSD1306（0x3C）地址不同，但某些山寨设备地址可能不一样。
5. 设备硬件损坏。

**诊断步骤**：
1. 使用万用表测量各引脚电压：
   - ESP32 的 VCC（3.3V）与 GND 之间应为 3.3V。
   - AHT20 / OLED 的 VCC 与 GND 之间也应为 3.3V。
   - SDA/SCL 引脚的静态电压应为 3.3V（由上拉电阻拉高），如果为 0V 则说明上拉缺失或短路。
2. 验证接线：SDA 连接 GPIO21，SCL 连接 GPIO22（ESP32 默认 I2C 引脚）。
3. 逐个测试设备：先只接一个 I2C 设备，运行扫描代码确认能否发现。
4. 在 `Wire.begin()` 后添加扫描代码（见第 5 章）。

**解决方案**：
- 在 SDA 和 SCL 线上各接一个 4.7kΩ 上拉电阻到 3.3V（如果杜邦线长度超过 20cm 则必须加装）。
- 缩短 I2C 总线长度（杜邦线尽量控制在 20cm 以内）。
- 检查 I2C 地址是否标准：部分山寨 SSD1306 可能使用 0x3D 地址。
- 如果 VCC 供电不足，OLED 和 AHT20 分别从 ESP32 的 3.3V 引脚取电，避免串联供电。
- 如果仍然无法检测到设备，考虑更换传感器/显示屏模块。

**代码示例 — 检查 I2C 地址**：
```cpp
#include <Wire.h>

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);  // SDA, SCL
    Serial.println("\nI2C Scanner");

    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("Found device at 0x%02X\n", addr);
        }
    }
    Serial.println("Scan complete.");
}

void loop() {}
```

---

### B010：OLED 花屏 / 不显示

**症状描述**：
OLED 屏幕通电后亮起（有背光）但无文字/图形显示，或者显示为满屏杂乱像素（"雪花"）、仅显示上半部分/下半部分。

**可能原因**：
1. I2C 通信不稳定（接线松动、线过长）。
2. OLED 初始化代码中指定的分辨率与实际屏幕不一致。
3. 屏幕复位时序不正确（SSD1306 需要特定的初始化序列）。
4. 屏幕地址配置错误（0x3C 与 0x3D 混淆）。
5. OLED 驱动 IC 并非 SSD1306（某些廉价屏幕可能使用 SH1106）。

**诊断步骤**：
1. 确认 I2C 地址：运行 I2C 扫描代码，确认设备地址（通常是 0x3C 或 0x3D）。
2. 检查初始化代码中的分辨率：如果是 128×64 的屏幕，使用 `SSD1306_128_64`；如果是 128×32 则使用 `SSD1306_128_32`。
3. 测试显示「全屏填充」模式，观察是否有像素异常区域。
4. 尝试在代码中增加 `display.begin()` 后的延时。

**解决方案**：
- 修正初始化代码中的地址参数：
  ```cpp
  #define OLED_ADDR 0x3C
  Adafruit_SSD1306 display(128, 64, &Wire, -1);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
      Serial.println("SSD1306 allocation failed");
  }
  ```
- 如果是 SH1106 驱动，需更换库或用兼容模式：
  ```cpp
  // 使用 Adafruit_SH110X 库
  Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);
  ```
- 在初始化后添加延时让屏幕稳定：
  ```cpp
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(500);
  display.clearDisplay();
  display.display();
  ```
- 检查接线，特别是 GND 连接必须可靠。
- 在 SDA/SCL 上加 4.7kΩ 上拉电阻（即使板载有上拉，并联增强信号）。

---

### B011：AHT20 读取失败 / 数据异常

**症状描述**：
串口输出的温度或湿度值为 `NaN`、`-999`、`0.00` 或明显不合理的数值（如温度 85℃、湿度 120%）。数据有时正常有时异常。

**可能原因**：
1. AHT20 未正确初始化（上电后需要至少 20ms 等待 + 校准命令）。
2. I2C 通信受到干扰（总线竞争、线缆过长）。
3. 传感器发热导致温度读数偏高（传感器靠近 ESP32 或电源模块）。
4. AHT20 处于休眠模式需要唤醒。
5. 多次连续读取间隔太短（AHT20 单次测量周期约 75ms）。

**诊断步骤**：
1. 分离传感器与热源（ESP32 的 WiFi 模块和稳压芯片会发热）。
2. 在两次读取之间加入至少 100ms 的延时。
3. 检查 I2C 地址：AHT20 的标准地址是 `0x38`（7 位地址左移 1 位后为 0x70）。
4. 使用第 5 章的 I2C 扫描脚本确认传感器是否在线。
5. 在干燥环境下与商用温湿度计对比数据。

**解决方案**：
- 正确的 AHT20 初始化序列：
  ```cpp
  #include <Adafruit_AHTX0.h>
  Adafruit_AHTX0 aht;

  void setup() {
      Serial.begin(115200);
      if (!aht.begin()) {
          Serial.println("AHT20 not found!");
          while (1) delay(10);
      }
      Serial.println("AHT20 initialized.");
  }
  ```
- 手动校准（传感器首次上电可能需要）：
  ```cpp
  // 发送 0xBE 校准命令（Adafruit 库已自动处理，无需手动）
  // 但如果使用原始 I2C 通信，需要在初始化时发送 0xBE
  ```
- 读取间隔控制：
  ```cpp
  void loop() {
      sensors_event_t humidity, temp;
      aht.getEvent(&humidity, &temp);
      Serial.printf("Temp: %.2f C, Humidity: %.2f %%\n",
                    temp.temperature, humidity.relative_humidity);
      delay(2000);  // 每 2 秒读取一次
  }
  ```
- 对读数进行软件滤波（移动平均或限幅滤波）：
  ```cpp
  // 简单限幅滤波：新值与前值相差超过阈值则丢弃
  float lastTemp = 0;
  float readTemperatureWithFilter(float newValue) {
      if (abs(newValue - lastTemp) > 10.0) {
          return lastTemp;  // 跳变过大，丢弃
      }
      lastTemp = newValue;
      return newValue;
  }
  ```

---

### B012：LED 不亮 / 颜色不对

**症状描述**：
三色 LED 完全不亮，或只亮某个颜色，或颜色与程序期望不符（例如代码要求绿色但实际显示红色）。

**可能原因**：
1. LED 引脚接线错误 — 共阴极/共阳极混淆。
2. 限流电阻阻值不匹配（阻值过大会导致亮度极低）。
3. GPIO 输出模式未正确设置（未设置为 `OUTPUT`）。
4. 对于共阴极 RGB LED，某个颜色通道损坏。
5. 引脚电流不够（ESP32 GPIO 最大输出约 12mA，三个通道同时亮可能超出）。

**诊断步骤**：
1. 确认 LED 类型：使用万用表二极管档或 3.3V 串联 220Ω 电阻测试：
   - 共阴极：公共脚接 GND，单独触发电 R/G/B 脚应亮起。
   - 共阳极：公共脚接 VCC（3.3V），单独触摸 GND 到 R/G/B 脚应亮起。
2. 检查限流电阻：红色通常用 220Ω，绿色和蓝色用 100Ω（不同颜色 LED 正向压降不同）。
3. 使用第 5 章的 GPIO 自检脚本逐引脚测试。
4. 测量 GPIO 输出电平：代码设置 HIGH 时万用表应测到 ~3.3V。

**解决方案**：
- 正确接线（假设共阴极 RGB LED，公共脚接 GND）：
  | LED 引脚 | 连接目标 | 限流电阻 |
  |---------|---------|---------|
  | 公共阴极 | GND | 无 |
  | R（红） | GPIO 16 | 220Ω |
  | G（绿） | GPIO 17 | 100Ω |
  | B（蓝） | GPIO 18 | 100Ω |
- 代码正确配置：
  ```cpp
  #define PIN_LED_R 16
  #define PIN_LED_G 17
  #define PIN_LED_B 18

  void setup() {
      pinMode(PIN_LED_R, OUTPUT);
      pinMode(PIN_LED_G, OUTPUT);
      pinMode(PIN_LED_B, OUTPUT);
  }

  void setColor(bool r, bool g, bool b) {
      // 共阴极 LED：HIGH = 亮
      digitalWrite(PIN_LED_R, r ? HIGH : LOW);
      digitalWrite(PIN_LED_G, g ? HIGH : LOW);
      digitalWrite(PIN_LED_B, b ? HIGH : LOW);
  }

  void setGreen() { setColor(LOW, HIGH, LOW); }
  ```
- 如果亮度不均衡，调整限流电阻值使三色视觉亮度一致。
- 对于共阳极 LED，逻辑反相：`digitalWrite(pin, color ? LOW : HIGH)`。

---

### B013：蜂鸣器不响

**症状描述**：
程序触发蜂鸣器（如湿度超限报警）时，蜂鸣器无声。其他功能（显示、LED 等）正常工作。

**可能原因**：
1. 蜂鸣器类型弄错 — 有源蜂鸣器只需要直流电平驱动，无源蜂鸣器需要 PWM 信号。
2. 蜂鸣器工作电压高于 GPIO 输出（3.3V）— 需要三极管驱动。
3. GPIO 驱动电流不足（蜂鸣器通常需要 20-50mA）。
4. 蜂鸣器正负极接反（部分有源蜂鸣器有极性要求）。
5. 代码中驱动逻辑错误（对有源蜂鸣器使用 `tone()` 函数，而对无源只用 `digitalWrite()`）。

**诊断步骤**：
1. 确认蜂鸣器类型：
   - 有源：通电即响（测量电阻：通常 16Ω 或 32Ω）。
   - 无源：需要 PWM 方波驱动（用万用表测量电阻通常 >100Ω）。
2. 用 3.3V 直接触碰蜂鸣器两个引脚——有源蜂鸣器应发出持续响声。
3. 用万用表测量驱动引脚电平：代码设置为 HIGH 时应为 3.3V。
4. 检查驱动电路：如果蜂鸣器是 5V 规格，需使用 NPN 三极管（如 S8050）驱动。

**解决方案**：
- 有源蜂鸣器（本项目使用的是有源蜂鸣器）：
  ```cpp
  #define BUZZER_PIN 19

  void setup() {
      pinMode(BUZZER_PIN, OUTPUT);
  }

  void alarmOn() {
      digitalWrite(BUZZER_PIN, HIGH);
  }

  void alarmOff() {
      digitalWrite(BUZZER_PIN, LOW);
  }
  ```
- 根据蜂鸣器规格调整驱动方式：
  - 3.3V 有源蜂鸣器：可直接由 GPIO 驱动（串联 100Ω 限流）。
  - 5V 有源蜂鸣器：需外接 NPN 三极管（S8050）驱动电路：
    ```
    ESP32 GPIO → 1kΩ 电阻 → 三极管基极
    三极管集电极 → 蜂鸣器负极
    蜂鸣器正极 → 5V VCC
    三极管发射极 → GND
    ```
- 如果蜂鸣器是 5V 且直接连接，需要将 VCC 接 5V 引脚（ESP32 的 VIN 或外部 5V），GPIO 通过三极管控制。

---

### B014：继电器不吸合

**症状描述**：
当湿度触发阈值时，继电器应吸合（发出清脆的"咔嗒"声，指示灯亮起），但实际上无任何反应。加湿器不工作。

**可能原因**：
1. 继电器模块供电不足 — 大多数继电器模块需要 5V 供电，而 ESP32 的 3.3V 引脚无法驱动。
2. GPIO 控制电平与继电器模块逻辑不匹配（高电平触发 vs 低电平触发）。
3. 继电器模块的输入引脚未接上拉/下拉，处于悬空状态。
4. 驱动三极管损坏。
5. 代码中控制逻辑错误（例如写反了 HIGH/LOW）。

**诊断步骤**：
1. 检查继电器模块供电 — VCC 应接 ESP32 的 VIN（5V）引脚而非 3.3V 引脚。
2. 用万用表测量继电器模块 JD-VCC 与 GND 之间电压，应为 5V。
3. 用万用表测量控制引脚（IN）电平：代码设置吸合时应为 HIGH（~3.3V）。
4. 手动用跳线将 IN 引脚短接到 VCC（5V）或 GND，观察继电器是否吸合，以此确认模块好坏。
5. 听继电器动作声音 — 有"咔嗒"声但加湿器不工作说明继电器正常，问题在加湿器接线。

**解决方案**：
- 确保继电器模块的 VCC 接到 5V 电源（ESP32 的 VIN 引脚或外部 5V 电源），GND 与 ESP32 共地。
- 如果继电器模块是**低电平触发**（常见于大多数光耦隔离继电器模块）：
  ```cpp
  #define RELAY_PIN 4

  void setup() {
      pinMode(RELAY_PIN, OUTPUT);
      digitalWrite(RELAY_PIN, HIGH);  // 初始关闭（低电平触发时，HIGH = 关闭）
  }

  void relayOn() {
      digitalWrite(RELAY_PIN, LOW);   // 低电平触发吸合
  }

  void relayOff() {
      digitalWrite(RELAY_PIN, HIGH);
  }
  ```
- 如果继电器模块是**高电平触发**：
  ```cpp
  void relayOn()  { digitalWrite(RELAY_PIN, HIGH); }
  void relayOff() { digitalWrite(RELAY_PIN, LOW);  }
  ```
- 不确定触发方式时，用一个 LED 串联电阻接到控制引脚上辅助观察：LED 亮时代表 GPIO 为 HIGH。
- 为继电器模块的控制引脚增加 10kΩ 下拉电阻，防止上电瞬间误触发。

---

### B015：按钮无反应 / 抖动严重

**症状描述**：
按下按钮（模式切换键或恢复默认键）时，程序无响应，或者按下一次却触发了多次动作（模式在多个状态间跳跃）。

**可能原因**：
1. 按钮接线错误 — 未接上拉/下拉电阻，引脚悬空电平不稳定。
2. 未做软件去抖处理（机械按钮按下时会产生 5-20ms 的抖动信号）。
3. 代码中 GPIO 中断或轮询逻辑错误。
4. 按钮引脚被 Strapping 引脚功能影响。
5. 按钮物理损坏（内部触点氧化或弹簧失效）。

**诊断步骤**：
1. 用万用表测量按钮按下前后引脚电平的变化。
2. 在 `setup()` 中将按钮引脚设置为 `INPUT_PULLUP` 或 `INPUT_PULLDOWN` 模式。
3. 在串口调试中输出引脚状态，观察按下和松开时的电平变化。
4. 检查按钮接线：一端接 GPIO，另一端接 GND（上拉模式）或 VCC（下拉模式）。

**解决方案**：
- 使用内部上拉电阻（最简单可靠）：
  ```cpp
  #define BTN_A 13  // 模式切换
  #define BTN_B 14  // 恢复默认

  void setup() {
      pinMode(BTN_A, INPUT_PULLUP);
      pinMode(BTN_B, INPUT_PULLUP);
  }
  ```
- 软件去抖（延时法）：
  ```cpp
  void loop() {
      static unsigned long lastPressA = 0;
      if (digitalRead(BTN_A) == LOW) {        // 按下（上拉模式，按下为 LOW）
          unsigned long now = millis();
          if (now - lastPressA > 200) {        // 200ms 去抖
              lastPressA = now;
              onButtonAPressed();
          }
      }
  }
  ```
- 更稳健的去抖方式 — 使用 Bounce2 库：
  ```cpp
  #include <Bounce2.h>
  Bounce buttonA = Bounce();

  void setup() {
      buttonA.attach(BTN_A, INPUT_PULLUP);
      buttonA.interval(25);  // 25ms 去抖
  }

  void loop() {
      buttonA.update();
      if (buttonA.fell()) {  // 检测到下降沿（按下）
          onButtonAPressed();
      }
  }
  ```
- 避免使用 ESP32 的 Strapping 引脚（GPIO 0、2、5、12、15）作为按钮引脚，因为它们在上电时的电平状态会影响芯片启动。

---

## 3. 运行时问题

---

### B016：程序卡死 / 看门狗重启

**症状描述**：
设备运行一段时间（几秒到几小时不等）后停止响应，OLED 画面定格，LED 不再变化，然后 ESP32 自动重启。串口输出中出现 `Guru Meditation Error`、`Task watchdog got triggered` 或 `Brownout detector was triggered`。

**可能原因**：
1. 程序中存在死循环（如 `while(1);` 或 `while(!condition);` 且条件永不满足）。
2. 阻塞式延时（`delay()`）过长，导致后台任务（WiFi、TCP/IP 堆栈）饿死。
3. 内存泄漏导致堆空间耗尽，`malloc` 返回 NULL，后续访问导致异常。
4. 低电压导致棕色检测（Brownout Detector）触发 — 供电不足。
5. 硬件 I2C 总线死锁 — SCL 被外设拉低。

**诊断步骤**：
1. 查看串口输出的错误信息：
   - `Guru Meditation Error: Core 1 panic'ed (Interrupt wdt timeout)` — 看门狗超时。
   - `Brownout detector was triggered` — 供电电压低于 2.5V。
   - `Unhandled interrupt` — 中断服务程序异常。
2. 注释掉部分功能，逐步排除直到找到引发崩溃的代码块。
3. 检查主循环中是否有超过 5 秒的 `delay()` 调用。
4. 使用内存监控代码（见第 5 章）观察堆内存变化趋势。

**解决方案**：
- 避免在主循环中使用长 `delay()`，改用 `millis()` 非阻塞定时架构：
  ```cpp
  unsigned long prevLoop = 0;
  const unsigned long INTERVAL = 2000;  // 2 秒

  void loop() {
      unsigned long now = millis();
      if (now - prevLoop >= INTERVAL) {
          prevLoop = now;
          readSensors();
          updateDisplay();
          updateLED();
          handleRelay();
      }
      // 处理按钮响应（非阻塞）
      handleButtons();
      // WiFi 后台任务（V2.0）
      handleWiFi();
  }
  ```
- 增加看门狗喂狗指令（如果启用了硬件看门狗）：
  ```cpp
  #include "esp_task_wdt.h"

  void setup() {
      esp_task_wdt_init(10, true);  // 10 秒超时
      esp_task_wdt_add(NULL);
  }

  void loop() {
      // ... 主要工作 ...
      esp_task_wdt_reset();  // 喂狗
  }
  ```
- 解决 Brownout 问题：
  - 使用 5V/2A 以上的电源适配器供电。
  - 在 ESP32 的 VCC 和 GND 之间并联 100μF 电解电容 + 0.1μF 陶瓷电容。
  - 在 `menuconfig` 或 `build_flags` 中禁用 Brownout 检测（风险较大，仅用于调试）：
    ```ini
    build_flags = -DCONFIG_BROWNOUT_DET=0
    ```

---

### B017：内存不足导致崩溃

**症状描述**：
程序刚启动时正常，运行一段时间后出现异常（显示异常、WiFi 断开、传感器读不到），最终崩溃重启。串口输出 `[E][esp32-hal-psram.c:xxx] malloc() failed` 或 `Out of memory`。

**可能原因**：
1. 代码中存在内存泄漏 — `malloc()` / `new` 申请内存后未 `free()` / `delete`。
2. 动态字符串操作（`String` 类）频繁拼接产生大量碎片。
3. 创建了大型局部变量（例如 `char buf[4096]`）导致栈溢出。
4. 全局变量和静态缓冲区过多，占用了大量 DRAM。
5. 启用了 OTA（空中升级）需要额外缓冲区。

**诊断步骤**：
1. 在串口循环输出堆内存信息，观察变化趋势（参见第 5 章监控代码）。
2. 检查代码中所有 `new` / `malloc` / `strdup` 是否有对应的 `delete` / `free`。
3. 尽量避免在循环中使用 `String` 类的 `+` 或 `+=` 操作。
4. 检查是否有递归调用可能造成栈溢出。

**解决方案**：
- 测量空闲堆内存：
  ```cpp
  Serial.printf("Free heap: %d bytes\n", esp_get_free_heap_size());
  Serial.printf("Minimum ever free: %d bytes\n", esp_get_minimum_free_heap_size());
  ```
- 避免在 loop 中使用 `String` 拼接：
  ```cpp
  // 错误方式 — 频繁创建临时 String 对象
  String msg = "Temp: " + String(temp) + "C, Humidity: " + String(hum) + "%";

  // 正确方式 — 使用 printf/sprintf 格式化
  char msg[64];
  snprintf(msg, sizeof(msg), "Temp: %.1f C, Humidity: %.1f %%", temp, hum);
  ```
- 如果内存持续下降，检查动态分配：
  ```cpp
  void loop() {
      char* buf = (char*)malloc(256);
      if (buf) {
          // 使用 buf...
          free(buf);  // 务必释放！
      }
  }
  ```
- 将大的静态数据放入 PSRAM（如果开发板有 PSRAM）：
  ```ini
  build_flags = -DBOARD_HAS_PSRAM -mfix-esp32-psram-cache-issue
  ```

---

### B018：定时不准确

**症状描述**：
传感器读数间隔、OLED 刷新频率、继电器控制等定时行为与预期不符。例如设置 2 秒刷新一次但实际约 4 秒才刷新一次，或者执行越来越快/越来越慢。

**可能原因**：
1. 使用了 `delay()` 阻塞延时，且其他操作的耗时累计影响了总循环时间。
2. `millis()` 返回值溢出（约 50 天后归零，但本项目运行时间短暂不考虑）。
3. `loop()` 中某个操作耗时很长（如 WiFi 连接、写 EEPROM）导致其他定时任务被延误。
4. 定时器优先级设置不当（FreeRTOS 任务抢占）。

**诊断步骤**：
1. 在 `loop()` 开头和结尾添加时间戳输出，观察每次循环的实际耗时。
2. 检查所有定时相关的 `delay()` 和 `millis()` 计算。
3. 使用示波器或逻辑分析仪测量某个 GPIO 切换的周期（如每 1 秒切换一次 LED 状态）。

**解决方案**：
- 使用非阻塞定时架构（基于 `millis()` 的状态机）：
  ```cpp
  const unsigned long SENSOR_INTERVAL = 2000;   // 2 秒读传感器
  const unsigned long DISPLAY_INTERVAL = 500;   // 0.5 秒刷 OLED
  const unsigned long RELAY_INTERVAL = 5000;    // 5 秒检测继电器

  unsigned long lastSensor = 0, lastDisplay = 0, lastRelay = 0;

  void loop() {
      unsigned long now = millis();

      if (now - lastSensor >= SENSOR_INTERVAL) {
          lastSensor = now;
          readSensor();
      }
      if (now - lastDisplay >= DISPLAY_INTERVAL) {
          lastDisplay = now;
          updateDisplay();
      }
      if (now - lastRelay >= RELAY_INTERVAL) {
          lastRelay = now;
          updateRelay();
      }
  }
  ```
- 使用硬件定时器（Ticker 库）替代软件定时：
  ```cpp
  #include <Ticker.h>

  Ticker sensorTicker;
  volatile bool sensorReady = false;

  void IRAM_ATTR onSensorTimer() {
      sensorReady = true;  // 在 ISR 中设置标记，主循环读取
  }

  void setup() {
      sensorTicker.attach(2.0, onSensorTimer);  // 每 2 秒触发一次
  }

  void loop() {
      if (sensorReady) {
          sensorReady = false;
          readSensor();
      }
  }
  ```
- 精确微秒级延时使用 `delayMicroseconds()`（但会阻塞 CPU，只用于短暂等待）。

---

### B019：传感器数据跳变

**症状描述**：
温湿度读数偶尔出现明显异常值（如从 25℃ 跳变到 85℃ 再跳回），导致继电器频繁开关、OLED 显示闪烁显示异常数字。

**可能原因**：
1. I2C 通信受到瞬时干扰（电源波动、电机启停、WiFi 发射峰值）。
2. AHT20 传感器的噪声（每次测量有一定随机误差）。
3. 两个传感器读取操作互相干扰（共用一个 I2C 总线时）。
4. 读取时序不对 — 在传感器未完成测量时读取数据。
5. 供电电压瞬间波动，影响传感器 ADC 参考电压。

**诊断步骤**：
1. 在串口监视器中持续观察数据，找到异常值出现的规律（是否与 WiFi 发包、继电器吸合同时发生？）。
2. 增加读取频率，对比相邻两次数据是否有跳变。
3. 使用示波器观察 I2C 总线 SDA/SCL 信号质量。

**解决方案**：
- 软件滤波（移动平均法）：
  ```cpp
  #define FILTER_SIZE 5
  float tempBuffer[FILTER_SIZE];
  int tempIndex = 0;

  float getFilteredTemperature(float newReading) {
      tempBuffer[tempIndex % FILTER_SIZE] = newReading;
      tempIndex++;
      float sum = 0;
      int count = min(tempIndex, FILTER_SIZE);
      for (int i = 0; i < count; i++) sum += tempBuffer[i];
      return sum / count;
  }
  ```
- 限幅滤波（丢弃跳变超过阈值的值）：
  ```cpp
  float lastValidTemp = 25.0;

  float limitFilter(float newValue) {
      if (abs(newValue - lastValidTemp) > 8.0) {
          return lastValidTemp;  // 丢弃该值
      }
      lastValidTemp = newValue;
      return newValue;
  }
  ```
- 在读取温湿度后添加短延时再访问：
  ```cpp
  aht.getEvent(&humidity, &temp);
  delay(5);  // 等待 I2C 总线稳定
  ```
- 在继电器吸合瞬间避免读取传感器（继电器线圈产生电磁干扰）：
  ```cpp
  void relayOn() {
      digitalWrite(RELAY_PIN, LOW);
      delay(50);  // 等电磁干扰过去
  }
  ```

---

### B020：OLED 刷新闪烁

**症状描述**：
OLED 屏幕上的数字或文字在更新时出现闪烁（整个画面闪一下或局部闪烁）。观察者可以明显感觉到屏幕有不连续的刷新感。

**可能原因**：
1. 每帧都执行 `display.clearDisplay()` + `display.display()`，导致画面先全黑再显示。
2. 绘制和显示之间的间隔过长，人眼觉察到屏幕熄灭的瞬间。
3. SSD1306 的 I2C 刷新率受限（约 30fps），复杂画面可能需要多次传输。
4. 主循环中其他耗时操作阻塞了 I2C 通信。

**诊断步骤**：
1. 检查代码中 `display.display()` 的调用频率。
2. 用逻辑分析仪测量 I2C 总线上数据传输的间隔。
3. 尝试注释掉其他耗时操作，仅保留显示刷新，观察闪烁是否消失。

**解决方案**：
- 减少 `clearDisplay()` 调用次数，改为只更新变化的部分：
  ```cpp
  void updateDisplay(float temp, float humidity) {
      static float lastTemp = -999;
      static float lastHum = -999;

      // 只在数值变化时才重绘
      if (abs(temp - lastTemp) > 0.1 || abs(humidity - lastHum) > 0.1) {
          display.clearDisplay();
          display.setCursor(0, 0);
          display.printf("Temp: %.1f C\nHum: %.1f %%", temp, humidity);
          display.display();

          lastTemp = temp;
          lastHum = humidity;
      }
  }
  ```
- 使用显示缓冲区 — 局部更新而非全屏重绘：
  ```cpp
  // 清除旧数值而非全屏清除
  void updateNumberOnly(float newValue, int x, int y) {
      display.fillRect(x, y, 60, 16, BLACK);  // 只擦除数字区域
      display.setCursor(x, y);
      display.printf("%.1f", newValue);
      display.display();
  }
  ```
- 提高 OLED 的 I2C 速度至 400kHz（Fast Mode）：
  ```cpp
  Wire.begin(21, 22, 400000);  // 第三个参数设置 I2C 时钟频率
  ```
- 在显示刷新期间关闭其他 I2C 操作，减少总线争抢。

---

### B021：继电器频繁开关

**症状描述**：
继电器在短时间内反复吸合、断开，发出连续"咔嗒"声。加湿器随之频繁启停，不仅噪音大，而且可能损坏继电器和加湿器。

**可能原因**：
1. 湿度阈值设置过于接近当前值，湿度在阈值上下反复波动。
2. 传感器数据跳变（见 B019）导致瞬间触发临界值。
3. 继电器控制逻辑缺少迟滞（滞回/Hysteresis）控制。
4. 控制周期过短（每次读取到数据就立即判断是否开关继电器）。

**诊断步骤**：
1. 观察串口输出的湿度值，看是否在阈值附近反复震荡。
2. 检查继电器控制逻辑中是否有延迟/最小开关间隔的保护。
3. 用示波器或逻辑分析仪记录继电器的控制引脚波形，观察开关频率。

**解决方案**：
- 加入迟滞（滞回）控制，防止阈值附近的频繁切换：
  ```cpp
  #define HUMIDITY_TARGET 55.0   // 目标湿度 55%
  #define HYSTERESIS 5.0         // 滞回带 ±5%

  void updateRelay(float currentHumidity) {
      static bool relayState = false;

      if (!relayState && currentHumidity < (HUMIDITY_TARGET - HYSTERESIS)) {
          // 湿度低于目标-滞回 → 开启加湿
          relayState = true;
          digitalWrite(RELAY_PIN, LOW);
          Serial.println("Relay ON");
      }
      else if (relayState && currentHumidity > (HUMIDITY_TARGET + HYSTERESIS)) {
          // 湿度高于目标+滞回 → 关闭加湿
          relayState = false;
          digitalWrite(RELAY_PIN, HIGH);
          Serial.println("Relay OFF");
      }
  }
  ```
- 添加继电器最小开关间隔保护：
  ```cpp
  void safeRelayControl(bool targetState) {
      static unsigned long lastToggle = 0;
      unsigned long now = millis();

      if (now - lastToggle < 10000) return;  // 10 秒内不允许再次切换

      digitalWrite(RELAY_PIN, targetState ? LOW : HIGH);
      lastToggle = now;
  }
  ```
- 不要每次传感器读取都判断继电器，控制周期建议至少 5-10 秒。
- 对传感器输入做滤波处理（滑动平均），减少瞬时跳变对继电器的影响。

---

### B022：程序运行正常但 OLED 一段时间后熄灭

**症状描述**：
设备刚启动时一切正常，OLED 显示完美。运行数小时后 OLED 突然熄灭（无显示），但程序其他功能（传感器读数、继电器控制）仍然正常。

**可能原因**：
1. SSD1306 驱动 I2C 通信死锁 — SCL 线被拉低导致总线阻塞。
2. OLED 模块内部电荷泵关闭（软件写入了睡眠命令）。
3. I2C 设备供电电压缓慢下降至欠压阈值以下。
4. OLED 模块过热保护（不常见，但在密闭环境中有可能）。

**诊断步骤**：
1. 测量 OLED VCC 引脚电压，查看是否低于 3.0V。
2. 在串口输出中加入 `display.begin()` 返回值检测循环。
3. 重置 I2C 总线（重新调用 `Wire.begin()`）观察 OLED 是否恢复。
4. 用逻辑分析仪捕获 I2C 总线，检查 SCL/SDA 信号是否异常。

**解决方案**：
- 在 loop 中周期性地检测 OLED 状态，必要时重新初始化：
  ```cpp
  void checkDisplay() {
      static unsigned long lastCheck = 0;
      if (millis() - lastCheck < 60000) return;  // 每分钟检查一次
      lastCheck = millis();

      // 尝试写入测试命令
      Wire.beginTransmission(0x3C);
      if (Wire.endTransmission() != 0) {
          Serial.println("OLED lost! Reinitializing...");
          display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
          display.clearDisplay();
          display.display();
      }
  }
  ```
- 在代码中避免向 OLED 发送睡眠命令（`ssd1306_command(SSD1306_DISPLAYOFF)`）。
- 在 VCC 线上并联一个 10μF 电容，防止供电瞬间跌落。
- 如果问题持续，考虑在硬件上加装 I2C 总线复位电路（将 SCL 通过 MOS 管接到 GND 实现总线复位）。

---

## 4. WiFi 问题（V2.0）

---

### B023：WiFi 连接失败

**症状描述**：
ESP32 无法连接到指定的 WiFi 路由器。串口输出持续显示 `Attempting to connect to SSID...` 最终超时并输出 `Failed to connect to WiFi`。

**可能原因**：
1. WiFi SSID 或密码错误（大小写敏感、含特殊字符）。
2. ESP32 与路由器距离过远或障碍物过多。
3. 路由器设置了 MAC 地址过滤。
4. WiFi 频段不匹配（ESP32 仅支持 2.4GHz，不支持 5GHz）。
5. 路由器开启了 WiFi 隐身（SSID 隐藏），需要手动指定 SSID。
6. ESP32 的 WiFi 模块天线接触不良（PCB 天线版）。

**诊断步骤**：
1. 检查串口输出的连接日志，确认尝试连接的 SSID 是否正确。
2. 用手机搜索 WiFi 信号，确认路由器 2.4GHz 频段是否开启。
3. 将 ESP32 放到距离路由器 1 米以内测试，排除信号强度问题。
4. 检查路由器管理界面中是否有 MAC 过滤白名单。
5. 尝试用手机热点连接 ESP32，排除路由器兼容性问题。

**解决方案**：
- 正确配置 WiFi（硬编码凭据）：
  ```cpp
  #include <WiFi.h>

  const char* ssid = "Your_SSID";
  const char* password = "Your_Password";

  void connectWiFi() {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);

      Serial.printf("Connecting to %s", ssid);
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
          attempts++;
          if (attempts > 40) {  // 20 秒超时
              Serial.println("\nFailed to connect!");
              return;
          }
      }
      Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
  }
  ```
- 添加重连机制：
  ```cpp
  void loop() {
      if (WiFi.status() != WL_CONNECTED) {
          Serial.println("WiFi disconnected, reconnecting...");
          connectWiFi();
      }
      // ... 其他任务 ...
  }
  ```
- 使用 WiFiManager 库（V2.0 更方便的方式，可通过 Web 页面配置 WiFi）：
  ```cpp
  #include <WiFiManager.h>

  void setup() {
      WiFiManager wm;
      wm.setConfigPortalTimeout(180);
      if (!wm.autoConnect("SmartHumidifier-AP")) {
          Serial.println("Failed to connect and timeout");
          ESP.restart();
      }
      Serial.println("WiFi connected!");
  }
  ```
- 在 `platformio.ini` 中确保启用了 WiFi 功能（默认即启用）。

---

### B024：WiFi 频繁掉线

**症状描述**：
设备可以连接 WiFi，但每隔几分钟到几十分钟就会断开连接，然后自动重连。程序会卡在重连过程中，导致传感器和控制功能暂时失效。

**可能原因**：
1. ESP32 供电不足 — WiFi 发射功率高峰时电流可达 500mA，超过 USB 供电能力。
2. WiFi 信道干扰 — 附近有多个 WiFi 路由器使用相同信道。
3. DHCP 租约时间过短，路由器频繁回收 IP 地址。
4. ESP32 进入了省电模式（Modem-sleep）导致连接断开。
5. 路由器连接设备数量上限已达。

**诊断步骤**：
1. 在串口监视器观察重连时间间隔，是否有规律（如每小时一次 → DHCP 续租）。
2. 用手机或电脑连接同一 WiFi，测试是否同样掉线。
3. 在 `setup()` 中禁用 WiFi 省电模式。
4. 检查 ESP32 供电电压，特别是在 WiFi 发射时的电压波动。

**解决方案**：
- 禁用 WiFi 省电模式：
  ```cpp
  void setup() {
      WiFi.mode(WIFI_STA);
      WiFi.setSleep(false);  // 禁止 WiFi 睡眠模式
      // ... 后续连接代码 ...
  }
  ```
- 供电增强：
  - 使用 5V/2A 以上的独立电源适配器。
  - ESP32 的 VIN 引脚接外部电源，避免仅靠 USB 供电。
  - 在 VCC 和 GND 之间并联 470μF 电解电容。
- 减小 WiFi 发射功率以降低功耗（信号强度下降但稳定性提高）：
  ```cpp
  WiFi.setTxPower(WIFI_POWER_11dBm);  // 默认为 19.5dBm
  ```
- 使用静态 IP 地址，避免 DHCP 租约问题：
  ```cpp
  IPAddress local_IP(192, 168, 1, 100);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);

  if (!WiFi.config(local_IP, gateway, subnet)) {
      Serial.println("Static IP config failed");
  }
  ```
- 实现智能重连（不阻塞主循环）：
  ```cpp
  void handleWiFiReconnect() {
      static unsigned long lastAttempt = 0;
      if (WiFi.status() == WL_CONNECTED) return;

      if (millis() - lastAttempt > 30000) {  // 每 30 秒尝试一次
          lastAttempt = millis();
          Serial.println("Attempting WiFi reconnection...");
          WiFi.reconnect();
      }
  }
  ```

---

### B025：Web 页面打不开

**症状描述**：
ESP32 已经成功连接 WiFi，串口输出了 IP 地址。在浏览器中输入该 IP 地址（如 `192.168.1.100`），浏览器显示「无法访问此网站」或「连接超时」。

**可能原因**：
1. Web 服务器未在代码中正确启动（`server.begin()` 缺失）。
2. 仅监听了本地回环地址（`127.0.0.1`）而非所有接口（`0.0.0.0`）。
3. 防火墙阻止了入站连接。
4. 浏览器与 ESP32 不在同一网段（跨子网）。
5. Web 服务端口被占用了不是标准 80 端口。
6. 代码中 server 处理逻辑阻塞，无法响应新请求。

**诊断步骤**：
1. 从 ESP32 串口输出确认获取到的 IP 地址。
2. 在电脑的命令行中 `ping 192.168.1.100`（ESP32 的 IP），确认网络可达。
3. 用电脑浏览器访问 `http://192.168.1.100:80` 或 `http://192.168.1.100:8080`（取决于代码中的端口）。
4. 检查代码中是否调用了 `server.handleClient()` 或 `server.begin()`。
5. 用手机连接同一 WiFi 并访问，排除电脑防火墙问题。

**解决方案**：
- 确保浏览器使用的是 HTTP 而非 HTTPS（ESP32 不支持 HTTPS 默认）。
- 在访问时明确带上端口号：
  ```
  http://192.168.1.100:80
  ```
- 正确的 Web 服务器代码框架：
  ```cpp
  #include <WiFi.h>
  #include <WebServer.h>

  WebServer server(80);

  void handleRoot() {
      server.send(200, "text/html",
          "<html><body><h1>SmartHumidifier</h1>"
          "<p>Temperature: 25.5 C</p>"
          "<p>Humidity: 55.2 %</p>"
          "</body></html>");
  }

  void setup() {
      // ... WiFi 连接 ...
      server.on("/", handleRoot);
      server.begin();  // 启动服务器
      Serial.println("HTTP server started");
  }

  void loop() {
      server.handleClient();  // 必须在 loop 中反复调用
  }
  ```
- 如果服务器被阻塞导致无响应，确保 `server.handleClient()` 放在主循环中不被阻塞的位置。
- 对于 AsyncWebServer，不需要 `handleClient()`，但需确保无阻塞：
  ```cpp
  #include <ESPAsyncWebServer.h>
  AsyncWebServer server(80);

  void setup() {
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
          request->send(200, "text/plain", "Hello");
      });
      server.begin();
  }
  ```

---

### B026：API 返回错误

**症状描述**：
通过 HTTP API 获取温湿度数据时，浏览器或程序收到错误响应（状态码 404、500 或返回空数据）。API 端点返回非 JSON 格式、或 JSON 格式错误导致解析失败。

**可能原因**：
1. API 路由注册错误（`server.on()` 中的路径与实际请求路径不匹配）。
2. HTTP 方法不匹配（代码注册了 GET 但客户端发送 POST）。
3. JSON 序列化格式错误（字符串缺少引号、逗号位置不对）。
4. API 处理函数中访问了未初始化的传感器数据。
5. CORS 头缺失导致浏览器端跨域请求被拒绝。

**诊断步骤**：
1. 用浏览器直接访问 API 地址，观察返回的原始内容（而非渲染结果）。
2. 使用 `curl` 或 Postman 工具发送精确的 HTTP 请求，查看响应头和状态码。
3. 在串口日志中输出每次 API 请求的处理详情。
4. 检查 JSON 格式是否正确。

**解决方案**：
- 标准 RESTful API 实现：
  ```cpp
  server.on("/api/status", HTTP_GET, []() {
      String json = "{";
      json += "\"temperature\":" + String(temp, 1) + ",";
      json += "\"humidity\":" + String(humidity, 1) + ",";
      json += "\"relayState\":" + String(relayState ? "true" : "false");
      json += "}";

      server.send(200, "application/json", json);
  });
  ```
- 使用 ArduinoJson 库（推荐，防止手拼 JSON 出错）：
  ```cpp
  #include <ArduinoJson.h>

  server.on("/api/status", HTTP_GET, []() {
      StaticJsonDocument<256> doc;
      doc["temperature"] = temp;
      doc["humidity"] = humidity;
      doc["relayState"] = relayState;

      String response;
      serializeJson(doc, response);
      server.send(200, "application/json", response);
  });
  ```
- 添加 CORS 头（如果 Web 页面是从其他域名加载的）：
  ```cpp
  server.on("/api/status", HTTP_GET, []() {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      // ... 后续处理 ...
  });
  ```
- 添加错误处理，API 中传感器数据无效时返回明确错误码：
  ```cpp
  server.on("/api/status", HTTP_GET, []() {
      if (isnan(temp) || isnan(humidity)) {
          server.send(503, "application/json",
              "{\"error\":\"Sensor data not available\"}");
          return;
      }
      // ... 正常响应 ...
  });
  ```

---

## 5. 诊断工具

---

### 5.1 串口日志分析脚本

以下脚本可帮助分析串口日志，计算程序运行时间、检测异常事件。

```cpp
// 在程序的关键节点添加带时间戳的日志
#define LOG_INFO(fmt, ...) \
    Serial.printf("[%10lu][INFO] " fmt "\n", millis(), ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) \
    Serial.printf("[%10lu][ERROR] " fmt "\n", millis(), ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) \
    Serial.printf("[%10lu][DEBUG] " fmt "\n", millis(), ##__VA_ARGS__)

void setup() {
    Serial.begin(115200);
    LOG_INFO("System starting...");
}

void loop() {
    static int loopCount = 0;
    loopCount++;

    if (loopCount % 100 == 0) {
        LOG_DEBUG("Loop count: %d", loopCount);
    }
}
```

在 PC 端接收日志后，可以用以下 PowerShell 简单过滤（Windows）：
```powershell
# 过滤出 ERROR 级别日志
Select-String -Path serial_log.txt -Pattern "\[ERROR\]"

# 统计不同事件的频率
Select-String -Path serial_log.txt -Pattern "\[INFO\] Relay (ON|OFF)" | Group-Object
```

---

### 5.2 I2C 扫描脚本

用于诊断 I2C 总线上的设备，确认 AHT20 和 SSD1306 是否被正确识别。

```cpp
/**
 * I2C_Scanner.ino — 用于 SmartHumidifier 项目
 * 功能：扫描 I2C 总线，列出所有设备地址
 */
#include <Wire.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n====================================");
    Serial.println("SmartHumidifier I2C Scanner");
    Serial.println("====================================");

    Wire.begin(21, 22);  // ESP32 默认 SDA=21, SCL=22
    Serial.printf("I2C speed: %d Hz\n", Wire.getClock());

    byte error, address;
    int deviceCount = 0;

    for (address = 0x01; address < 0x7F; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.printf("  [OK] Device found at 0x%02X", address);
            // 识别已知设备
            if (address == 0x38) Serial.print(" <- AHT20");
            if (address == 0x3C) Serial.print(" <- SSD1306 OLED");
            Serial.println();
            deviceCount++;
        }
        else if (error == 4) {
            Serial.printf("  [ERR] Unknown error at 0x%02X\n", address);
        }
    }

    if (deviceCount == 0) {
        Serial.println("No I2C devices found! Check wiring.");
    } else {
        Serial.printf("Found %d device(s).\n", deviceCount);
    }
    Serial.println("====================================");
}

void loop() {}
```

预期输出：
```
====================================
SmartHumidifier I2C Scanner
====================================
  [OK] Device found at 0x38 <- AHT20
  [OK] Device found at 0x3C <- SSD1306 OLED
Found 2 device(s).
====================================
```

---

### 5.3 GPIO 自检程序

用于逐一测试所有外设（LED、蜂鸣器、继电器、按钮），方便诊断硬件连接问题。

```cpp
/**
 * GPIO_SelfTest.ino — SmartHumidifier 硬件自检程序
 * 按顺序测试：LED -> 蜂鸣器 -> 继电器 -> 按钮
 */
#include <Wire.h>

// 引脚定义
#define PIN_LED_R   16
#define PIN_LED_G   17
#define PIN_LED_B   18
#define PIN_BUZZER  19
#define PIN_RELAY   4
#define PIN_BTN_A   13
#define PIN_BTN_B   14
#define PIN_SDA     21
#define PIN_SCL     22

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n===== GPIO Self Test =====");

    // 设置所有输出引脚
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_RELAY, OUTPUT);
    pinMode(PIN_BTN_A, INPUT_PULLUP);
    pinMode(PIN_BTN_B, INPUT_PULLUP);

    // 初始关闭所有输出
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, LOW);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_RELAY, HIGH);  // 低电平触发继电器，HIGH = 关闭

    Serial.println("Press Button A to start test...");
    Serial.println("Press Button B to skip current test...");
}

void testLEDs() {
    Serial.println("\n--- Test: RGB LED ---");
    Serial.println("Red ON (3s)...");
    digitalWrite(PIN_LED_R, HIGH); delay(3000); digitalWrite(PIN_LED_R, LOW);

    Serial.println("Green ON (3s)...");
    digitalWrite(PIN_LED_G, HIGH); delay(3000); digitalWrite(PIN_LED_G, LOW);

    Serial.println("Blue ON (3s)...");
    digitalWrite(PIN_LED_B, HIGH); delay(3000); digitalWrite(PIN_LED_B, LOW);

    Serial.println("White (all ON, 2s)...");
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, HIGH);
    digitalWrite(PIN_LED_B, HIGH);
    delay(2000);
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, LOW);
    Serial.println("LED test completed.");
}

void testBuzzer() {
    Serial.println("\n--- Test: Buzzer ---");
    Serial.println("Buzzer ON (2s)...");
    digitalWrite(PIN_BUZZER, HIGH);
    delay(2000);
    digitalWrite(PIN_BUZZER, LOW);
    Serial.println("Buzzer test completed.");
}

void testRelay() {
    Serial.println("\n--- Test: Relay ---");
    Serial.println("Relay ON (2s)...");
    digitalWrite(PIN_RELAY, LOW);   // 吸合
    delay(2000);
    Serial.println("Relay OFF (2s)...");
    digitalWrite(PIN_RELAY, HIGH);  // 断开
    delay(2000);
    Serial.println("Relay test completed.");
}

void testButtons() {
    Serial.println("\n--- Test: Buttons ---");
    Serial.println("Press Button A within 10 seconds...");
    unsigned long start = millis();
    bool detected = false;

    while (millis() - start < 10000) {
        if (digitalRead(PIN_BTN_A) == LOW) {
            Serial.println("Button A pressed!");
            delay(200);  // 简单去抖
            detected = true;
            break;
        }
        if (digitalRead(PIN_BTN_B) == LOW) {
            Serial.println("Button B pressed (skip)!");
            return;
        }
    }

    if (!detected) {
        Serial.println("Button A not detected.");
    }
}

void loop() {
    static int testPhase = 0;

    // 等待按钮 A 按下进入测试
    if (digitalRead(PIN_BTN_A) == LOW) {
        delay(50);  // 去抖
        while (digitalRead(PIN_BTN_A) == LOW);  // 等待松开

        switch (testPhase) {
            case 0: testLEDs(); break;
            case 1: testBuzzer(); break;
            case 2: testRelay(); break;
            case 3: testButtons(); break;
        }
        testPhase = (testPhase + 1) % 4;
    }
}
```

---

### 5.4 内存使用监控代码

用于监控堆内存使用情况，排查内存泄漏。

```cpp
/**
 * MemoryMonitor.ino — 监控 ESP32 堆内存使用
 * 可作为独立诊断程序，也可嵌入主程序循环中
 */
#include <esp_heap_caps.h>

struct MemoryInfo {
    size_t freeHeap;
    size_t minFreeHeap;
    size_t freePsram;        // PSRAM 空闲（如适用）
    size_t maxAllocHeap;     // 最大可分配连续内存块
};

MemoryInfo getMemoryInfo() {
    MemoryInfo info;
    info.freeHeap = esp_get_free_heap_size();
    info.minFreeHeap = esp_get_minimum_free_heap_size();
    info.maxAllocHeap = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    if (psramFound()) {
        info.freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    } else {
        info.freePsram = 0;
    }
    return info;
}

void printMemoryInfo() {
    MemoryInfo mem = getMemoryInfo();

    Serial.println("----- Memory Status -----");
    Serial.printf("Free heap:        %10d bytes\n", mem.freeHeap);
    Serial.printf("Min free heap:    %10d bytes\n", mem.minFreeHeap);
    Serial.printf("Largest block:    %10d bytes\n", mem.maxAllocHeap);
    if (mem.freePsram > 0) {
        Serial.printf("Free PSRAM:       %10d bytes\n", mem.freePsram);
    }

    // 计算内存压力指数
    float pressure = 100.0f * (1.0f - (float)mem.freeHeap / 327680);
    Serial.printf("Memory pressure:  %6.1f%%\n", pressure);
    if (mem.freeHeap < 20000) {
        Serial.println("WARNING: Low memory! Possible crash risk.");
    }
    Serial.println("--------------------------");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    printMemoryInfo();
}

void loop() {
    printMemoryInfo();

    // 模拟内存操作以测试监控效果
    char* leak = (char*)malloc(100);
    if (leak) {
        // 注意：故意不释放，观察内存下降趋势
        // 实际使用中应包含对应的 free(leak);
    }

    delay(5000);  // 每 5 秒输出一次
}
```

将此代码段插入主程序的 `loop()` 中可以实时监控内存变化：
```cpp
// 在主程序中每 60 秒记录一次内存状态
static unsigned long lastMemCheck = 0;
if (millis() - lastMemCheck > 60000) {
    lastMemCheck = millis();
    Serial.printf("Free heap: %d | Min: %d\n",
        esp_get_free_heap_size(),
        esp_get_minimum_free_heap_size());
}
```

---

### 5.5 综合诊断：一键运行所有自检

为方便快速排查，以下是整合了 I2C 扫描、GPIO 测试和内存检查的综合诊断入口代码：

```cpp
/**
 * diag.ino — SmartHumidifier 综合诊断模式
 * 在 setup() 中检测到特定引脚（如 Button B 按住）即进入诊断模式
 */
void enterDiagnosticMode() {
    Serial.println("\n========================================");
    Serial.println("SmartHumidifier Diagnostic Mode v1.0");
    Serial.println("========================================");

    // 1. I2C 扫描
    Serial.println("\n[1/4] I2C Bus Scan...");
    Wire.begin(21, 22);
    // ... I2C 扫描代码（见 5.2 节）...

    // 2. 内存状态
    Serial.println("\n[2/4] Memory Status...");
    printMemoryInfo();

    // 3. 引脚快速测试
    Serial.println("\n[3/4] Quick GPIO Test...");
    // LED 闪烁 + 蜂鸣器响一声
    for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_LED_R, HIGH);
        digitalWrite(PIN_BUZZER, HIGH);
        delay(200);
        digitalWrite(PIN_LED_R, LOW);
        digitalWrite(PIN_BUZZER, LOW);
        delay(200);
    }

    // 4. 系统信息
    Serial.println("\n[4/4] System Info...");
    Serial.printf("ESP32 Chip Rev:  %d\n", ESP.getChipRevision());
    Serial.printf("Chip Cores:      %d\n", ESP.getChipCores());
    Serial.printf("Flash Size:      %d\n", ESP.getFlashChipSize());
    Serial.printf("PSRAM:           %s\n", psramFound() ? "Yes" : "No");
    Serial.printf("SDK Version:     %s\n", ESP.getSdkVersion());

    Serial.println("\nDiagnostic complete. Exiting to normal operation.");
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BTN_B, INPUT_PULLUP);

    // 按住 Button B 上电进入诊断模式
    if (digitalRead(PIN_BTN_B) == LOW) {
        delay(100);
        if (digitalRead(PIN_BTN_B) == LOW) {
            enterDiagnosticMode();
            while (1);  // 诊断完成后停止
        }
    }

    // 正常启动...
}
```

---

## 6. 调试技巧

---

### 6.1 使用 Serial.print 调试

这是最简单也是最重要的调试手段。

**基本原则**：
- 在所有关键函数入口和出口添加打印语句。
- 输出变量值以验证程序逻辑。
- 使用有意义的标签区分不同模块的输出。

**技巧清单**：

```cpp
// 技巧 1：用宏开关控制调试输出（发布时一键关闭）
#define DEBUG_ENABLE 1

#if DEBUG_ENABLE
  #define DPRINT(fmt, ...) Serial.printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
  #define DPRINT(fmt, ...)  // 空宏，编译时消除
#endif

// 技巧 2：模块化标签
void loop() {
    DPRINT("[SENSOR] Starting read...");
    // ...
    DPRINT("[DISPLAY] Updating OLED...");
    // ...
    DPRINT("[RELAY] Checking threshold...");
}

// 技巧 3：输出变量名 + 值（方便日志分析）
int humidityLevel = 55;
DPRINT("humidityLevel = %d", humidityLevel);

// 技巧 4：用条件断点替代硬件断点
void readSensor() {
    sensors_event_t hum, temp;
    aht.getEvent(&hum, &temp);

    // 只在异常值时输出警告
    if (temp.temperature > 60 || temp.temperature < -10) {
        Serial.println("!!! WARNING: Temperature out of range !!!");
    }
    if (isnan(temp.temperature)) {
        Serial.println("!!! ERROR: Sensor returned NaN !!!");
        return;  // 及早返回，避免使用无效数据
    }
}
```

**调试输出样本**（通过串口监视器查看）：

```
[00000000][INFO] System starting...
[00000000][DEBUG] Wi-Fi mode set to STA
[00002018][DEBUG] Connecting to WiFi...
[00003522][INFO] WiFi connected, IP: 192.168.1.100
[00003522][SENSOR] Starting read...
[00003610][SENSOR] Temp: 25.3 C, Hum: 58.2%
[00003610][RELAY] Hum=58.2%, target=55.0%, hyst=5.0% -> NO ACTION
[00003610][DISPLAY] OLED updated
```

---

### 6.2 用 LED 指示程序运行状态

在没有串口或屏幕的情况下，使用板载 LED 或 RGB LED 指示运行状态是非常有效的调试手段。

```cpp
/**
 * LED 状态指示协议
 * 定义：不同的 LED 闪烁模式代表不同的运行状态
 */

// 状态枚举
enum SystemState {
    STATE_BOOTING,      // 启动中
    STATE_NORMAL,       // 正常运行
    STATE_SENSOR_ERR,   // 传感器错误
    STATE_WIFI_ERR,     // WiFi 错误
    STATE_ALARM         // 报警
};

SystemState currentState = STATE_BOOTING;

void updateStatusLed() {
    static unsigned long lastBlink = 0;
    static bool ledOn = false;
    unsigned long now = millis();
    unsigned long interval = 0;

    // 根据不同状态设定不同闪烁频率
    switch (currentState) {
        case STATE_BOOTING:
            interval = 100;     // 快闪（100ms 间隔）
            setColor(HIGH, LOW, LOW);   // 红色
            break;
        case STATE_NORMAL:
            interval = 2000;    // 慢闪（2 秒间隔）
            setColor(LOW, HIGH, LOW);   // 绿色
            break;
        case STATE_SENSOR_ERR:
            interval = 300;     // 中速闪烁
            setColor(HIGH, HIGH, LOW);  // 黄色（红+绿）
            break;
        case STATE_WIFI_ERR:
            interval = 500;     // 中速闪烁
            setColor(LOW, LOW, HIGH);   // 蓝色
            break;
        case STATE_ALARM:
            interval = 150;     // 急促闪烁
            setColor(HIGH, LOW, LOW);   // 红色
            break;
    }

    if (now - lastBlink >= interval) {
        lastBlink = now;
        ledOn = !ledOn;
        // 根据 ledOn 状态控制 RGB LED 亮灭
        if (ledOn) {
            // 保持 setColor 设置的颜色
        } else {
            setColor(LOW, LOW, LOW);  // 熄灭
        }
    }
}
```

**LED 状态速查表**：

| LED 状态 | 颜色 | 含义 | 操作建议 |
|---------|------|------|---------|
| 常亮 | 绿 | 一切正常 | 无需操作 |
| 慢闪 | 绿 | 正常，WiFi 连接中 | 等待 |
| 快闪 | 红 | 启动中/错误 | 查看串口日志 |
| 闪烁 | 黄 | 传感器异常 | 检查 I2C 接线 |
| 闪烁 | 蓝 | WiFi 异常 | 检查网络配置 |
| 急促闪烁 | 红 | 湿度报警 | 检查加湿设备 |
| 熄灭 | 无 | 未上电或程序未运行 | 检查电源 |

---

### 6.3 分段注释定位问题

当程序行为异常时，通过「注释排除法」快速锁定问题范围。

**操作步骤**：

1. **二分法注释**：将 `loop()` 中的代码分为前后两半，注释掉一半，看问题是否复现。
   ```cpp
   void loop() {
       // 注释掉前半部分
       // readSensors();
       // updateDisplay();
       // handleButtons();

       // 只保留后半部分
       updateRelay();
       updateStatusLed();
   }
   ```

2. **隔离外设驱动**：如果怀疑某个外设代码导致问题，将该外设的初始化代码移到 `setup()` 末尾，或将其功能注释掉。
   ```cpp
   void setup() {
       Serial.begin(115200);

       // 注释掉 OLED 初始化，看是否是 OLED 导致 I2C 总线问题
       // if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
       //     Serial.println("OLED init failed");
       // }

       aht.begin();  // 只保留 AHT20
   }
   ```

3. **逐层剥离**：建立最小可复现程序，从裸板代码开始逐层添加功能：
   ```
   步骤 1: Blink.ino（仅 LED 闪烁）→ 确认板子正常
   步骤 2: + I2C 扫描 → 确认总线正常
   步骤 3: + AHT20 读取 → 确认传感器正常
   步骤 4: + OLED 显示 → 确认显示正常
   步骤 5: + 继电器控制 → 确认执行器正常
   ```

4. **Serial.println 断点法**：在可疑位置前后添加打印，通过日志判断程序执行路径。
   ```cpp
   void someFunction() {
       Serial.println("[DEBUG] Entered someFunction()");
       // ... 中间代码 ...
       Serial.println("[DEBUG] Leaving someFunction()");
   }
   ```

5. **返回值检查**：对每个可能失败的函数检查返回值。
   ```cpp
   if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
       Serial.println("CRITICAL: OLED init failed! Freezing.");
       while (1);  // 程序暂停，方便观察
   }
   ```

---

### 6.4 使用示波器 / 逻辑分析仪（可选）

对于 I2C 通信问题、PWM 信号测量、GPIO 时序问题，示波器和逻辑分析仪是终极诊断工具。

**适用场景**：

| 工具 | 适用问题 | 用法 |
|-----|---------|------|
| 示波器 | 模拟信号质量、I2C 电平、电源纹波 | 测量 SDA/SCL 的波形幅度、上升沿时间 |
| 逻辑分析仪 | 数字信号时序、I2C 协议分析、GPIO 时序 | 解码 I2C 数据包、测量脉冲宽度 |

**入门级工具推荐**：
- **逻辑分析仪**：Saleae Logic 8（或国产克隆版，约 50-100 元），24MHz 采样率足够分析 I2C（400kHz）。
- **示波器**：FNIRSI 1013D（约 400 元）或 Hantek 6022BE（约 300 元）。

**用逻辑分析仪排查 I2C 问题的步骤**：

1. 将逻辑分析仪的通道 0 接 SDA，通道 1 接 SCL，GND 共地。
2. 设置采样率 4MHz（采集 I2C 400kHz 信号需至少 2MHz 以上）。
3. 触发条件设置为 SCL 下降沿或 I2C Start 条件。
4. 捕捉 I2C 通信过程，观察：
   - 地址字节是否正确发送（AHT20 地址 0x38 + 写位 = 0x70）。
   - 从设备是否返回 ACK（正常应看到第 9 位为低电平）。
   - 数据帧顺序是否正确。
   - SCL 时钟是否对称、是否有毛刺。

**用逻辑分析仪排查按钮/继电器时序的步骤**：

1. 将通道 0 接按钮引脚，通道 1 接继电器引脚。
2. 设置上升沿触发。
3. 按下按钮，观察：
   - 按钮按下时是否有抖动（理想应为干净的单次跳变，有抖动会看到多次跳变）。
   - 从按钮按下到继电器动作的延迟时间。
   - 继电器开关频率是否正常。

---

## 7. 附录：错误码速查

| 错误码 / 现象 | 可能原因 | 紧急程度 | 快速解决方案 |
|-------------|---------|---------|------------|
| `Brownout detector was triggered` | 供电电压不足 | 高 | 换 5V/2A 电源；加电容 |
| `Guru Meditation Error` | 内存访问越界/看门狗超时 | 高 | 检查数组越界；加喂狗 |
| `WiFi status: WL_DISCONNECTED` | WiFi 连接断开 | 中 | 检查 SSID/密码；加自动重连 |
| I2C 扫描无设备 | 接线错误/上拉缺失 | 高 | 检查 VCC/GND/SDA/SCL |
| OLED 无显示 | 地址/分辨率错误 | 中 | 运行 I2C 扫描确认地址 |
| 传感器读 NaN | 初始化失败/通信中断 | 中 | 复位传感器；检查接线 |
| 继电器不动作 | 供电/电平逻辑错误 | 中 | 确认是低电平触发还是高电平触发 |
| 按钮无反应 | 上拉配置/去抖缺失 | 低 | 使用 INPUT_PULLUP；加去抖 |
| 蜂鸣器不响 | 类型混淆/驱动不足 | 低 | 区分有源/无源；加三极管驱动 |
| 编译失败 `lib not found` | 库未安装/名称错误 | 高 | 在 lib_deps 中正确声明 |
| 烧录超时 connecting | 未进入下载模式 | 高 | 按住 BOOT+按 EN |

---

> **文档修订记录**
>
> | 版本 | 日期 | 修改内容 | 作者 |
> |-----|------|---------|------|
> | v1.0 | 2026-07-11 | 初版创建，涵盖 B001-B026 共 26 个 Bug 排查条目，含 4 个诊断工具和 4 个调试技巧 | SHC Team |
