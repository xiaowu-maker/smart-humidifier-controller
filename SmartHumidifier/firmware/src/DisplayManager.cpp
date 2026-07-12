/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * DisplayManager - OLED 显示管理模块（实现文件）
 * ============================================================
 */

#include "DisplayManager.h"
#include "config.h"
#include "Logger.h"

// ============================================================
// 初始化
// ============================================================

bool DisplayManager::begin() {
    LOG_INFO("DISPLAY", "Initializing SSD1306 OLED...");

    _initialized = false;
    _currentPage = 0;
    _lastUpdateTime = 0;

    // SSD1306 构造函数参数：
    //   128 = 宽度, 64 = 高度, &Wire = I2C 对象
    //   0x3C = I2C 地址, -1 = 无复位引脚
    _display = Adafruit_SSD1306(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

    if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        LOG_ERROR("DISPLAY", "SSD1306 not found at I2C address 0x3C");
        LOG_ERROR("DISPLAY", "Check: VCC=3.3V, SDA=GPIO21, SCL=GPIO22");
        return false;
    }

    // 初始显示设置
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);  // 单色屏，只有白和黑
    _display.setTextWrap(false);           // 不自动换行（超出的字会被裁掉）

    // 显示启动画面
    _display.setTextSize(1);
    _display.setCursor(20, 10);
    _display.println(F("Smart Humidifier"));
    _display.setCursor(35, 25);
    _display.println(F("Controller"));
    _display.setCursor(40, 40);
    _display.print(F("V"));
    _display.println(PROJECT_VERSION);
    _display.setCursor(30, 55);
    _display.println(F("Booting..."));
    _display.display();

    _initialized = true;
    LOG_INFO("DISPLAY", "SSD1306 OLED initialized OK");
    return true;
}

// ============================================================
// 刷新显示
// ============================================================

void DisplayManager::update(const SensorData& data, bool relayOn,
                             const char* statusText) {
    if (!_initialized) return;

    // 控制刷新频率，不每循环都刷
    if (millis() - _lastUpdateTime < DISPLAY_UPDATE_INTERVAL_MS) {
        return;
    }
    _lastUpdateTime = millis();

    _display.clearDisplay();

    // 根据当前页面调用对应的绘制函数
    switch (_currentPage) {
        case 0:
            _drawMainPage(data, relayOn, statusText);
            break;
        case 1:
            _drawNetworkPage();
            break;
        case 2:
            _drawSystemPage();
            break;
    }

    _drawPageIndicator();
    _display.display();
}

// ============================================================
// 页面控制
// ============================================================

void DisplayManager::nextPage() {
    _currentPage++;
    if (_currentPage > 2) _currentPage = 0;

    // 切换页面时强制刷新
    _lastUpdateTime = 0;
    LOG_DEBUG("DISPLAY", ("Switched to page " + String(_currentPage)).c_str());
}

uint8_t DisplayManager::getCurrentPage() {
    return _currentPage;
}

// ============================================================
// 特殊显示
// ============================================================

void DisplayManager::showError(const char* title, const char* message) {
    if (!_initialized) return;

    _display.clearDisplay();

    // 错误图标
    _display.setTextSize(2);
    _display.setCursor(55, 0);
    _display.println(F("!!"));

    // 标题
    _display.setTextSize(1);
    _display.setCursor(0, 20);
    _display.println(title);

    // 消息
    _display.setCursor(0, 35);
    _display.println(message);

    _display.display();
}

void DisplayManager::clear() {
    if (!_initialized) return;
    _display.clearDisplay();
    _display.display();
}

bool DisplayManager::isOK() {
    return _initialized;
}

// ============================================================
// 页面绘制 — Page 0: 主页面
// ============================================================

void DisplayManager::_drawMainPage(const SensorData& data, bool relayOn,
                                    const char* statusText) {
    // 布局（128×64）：
    // y=0:  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ (标题栏反白)
    // y=10: Smart Humidifier
    // y=18: ────────────────────────────── (分割线)
    // y=22: Humidity : 48.2 %       (大字)
    // y=42: Temp    : 26.3 C       (大字)
    // y=58: Status  : HEATING       (小字)

    // 标题栏（反白：白底黑字）
    _display.fillRect(0, 0, OLED_WIDTH, 12, SSD1306_WHITE);
    _display.setTextColor(SSD1306_BLACK);
    _display.setTextSize(1);
    _display.setCursor(20, 2);
    _display.println(F("Smart Humidifier"));
    _display.setTextColor(SSD1306_WHITE);  // 恢复白字

    // 分割线
    _display.drawFastHLine(0, 14, OLED_WIDTH, SSD1306_WHITE);

    // 湿度 — 使用大字体
    _display.setTextSize(2);
    _display.setCursor(0, 20);
    _display.print(F("H:"));
    if (data.valid) {
        _display.print(data.humidity, 1);
        _display.print(F("%"));
    } else {
        _display.print(F("--.-%"));
    }

    // 温度 — 大字体
    _display.setCursor(0, 40);
    _display.print(F("T:"));
    if (data.valid) {
        _display.print(data.temperature, 1);
    } else {
        _display.print(F("--.-"));
    }
    // 温度单位特殊处理：° 符号
    _display.setTextSize(1);
    _display.setCursor(115, 40);
    _display.print(F("o"));
    _display.setTextSize(2);
    _display.setCursor(121, 40);
    _display.print(F("C"));

    // 状态行
    _display.setTextSize(1);
    _display.setCursor(0, 57);
    _display.print(F("Status: "));
    _display.print(statusText);

    // 加湿器图标（简单的方块）
    if (relayOn) {
        _display.fillRect(120, 57, 7, 7, SSD1306_WHITE);
    } else {
        _display.drawRect(120, 57, 7, 7, SSD1306_WHITE);
    }
}

// ============================================================
// 页面绘制 — Page 1: 网络信息（V2.0 占位）
// ============================================================

void DisplayManager::_drawNetworkPage() {
    _drawHeader("Network Info");

    _display.setTextSize(1);
    _display.setCursor(0, 18);
    _display.println(F("WiFi: Not connected"));
    _display.setCursor(0, 30);
    _display.println(F("IP: ---.---.---.---"));
    _display.setCursor(0, 42);
    _display.println(F("RSSI: -- dBm"));
    _display.setCursor(0, 54);
    _display.println(F("(V2.0 feature)"));
}

// ============================================================
// 页面绘制 — Page 2: 系统信息
// ============================================================

void DisplayManager::_drawSystemPage() {
    _drawHeader("System Info");

    _display.setTextSize(1);
    _display.setCursor(0, 18);
    _display.print(F("Ver: V"));
    _display.println(PROJECT_VERSION);

    _display.setCursor(0, 30);
    _display.print(F("MCU: ESP32"));

    // 运行时间
    unsigned long uptime = millis() / 1000;
    int h = uptime / 3600;
    int m = (uptime % 3600) / 60;
    _display.setCursor(0, 42);
    _display.print(F("Up: "));
    _display.print(h);
    _display.print(F("h "));
    _display.print(m);
    _display.print(F("m"));

    _display.setCursor(0, 54);
    _display.print(F("Sensor: --"));
}

// ============================================================
// 工具函数
// ============================================================

void DisplayManager::_drawPageIndicator() {
    // 底部 3 个圆点表示 3 个页面，当前页是实心圆
    int dotY = OLED_HEIGHT - 2;
    int startX = OLED_WIDTH / 2 - 6;

    for (int i = 0; i < 3; i++) {
        int dotX = startX + i * 6;
        if (i == _currentPage) {
            _display.fillCircle(dotX, dotY, 2, SSD1306_WHITE);  // 实心
        } else {
            _display.drawCircle(dotX, dotY, 2, SSD1306_WHITE);  // 空心
        }
    }
}

void DisplayManager::_drawHeader(const char* title) {
    // 反白标题栏
    _display.fillRect(0, 0, OLED_WIDTH, 12, SSD1306_WHITE);
    _display.setTextColor(SSD1306_BLACK);
    _display.setTextSize(1);
    _display.setCursor(2, 2);
    _display.println(title);
    _display.setTextColor(SSD1306_WHITE);
    _display.drawFastHLine(0, 14, OLED_WIDTH, SSD1306_WHITE);
}

void DisplayManager::_drawDivider(int y) {
    _display.drawFastHLine(0, y, OLED_WIDTH, SSD1306_WHITE);
}
