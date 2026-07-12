/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * DisplayManager - OLED 显示管理模块（头文件）
 * ============================================================
 *
 * 职责：管理 0.96 寸 OLED 屏（SSD1306, I2C, 128×64）的一切显示操作。
 *
 * 屏幕：SSD1306 128×64 单色 OLED
 * 接口：I2C（地址 0x3C）
 * 字体：Adafruit_GFX 内置字体（5x7 像素/字符）
 *
 * 多页面设计：
 *   Page 0 - 主页面：温湿度 + 状态
 *   Page 1 - 网络信息：IP + RSSI（V2.0）
 *   Page 2 - 系统信息：版本 + 运行时间
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "SensorManager.h"

class DisplayManager {
public:
    // ============================================================
    // 生命周期
    // ============================================================

    /** 初始化 OLED，返回 true=成功 */
    bool begin();

    /**
     * 刷新显示内容
     * 内部自动判断刷新间隔和当前页面
     * @param data  传感器数据
     * @param relayOn  继电器是否开启
     * @param statusText  状态文本（如 "HEATING", "STANDBY"）
     */
    void update(const SensorData& data, bool relayOn, const char* statusText);

    // ============================================================
    // 页面控制
    // ============================================================

    /** 切换到下一个页面 */
    void nextPage();

    /** 获取当前页面编号 */
    uint8_t getCurrentPage();

    // ============================================================
    // 特殊显示
    // ============================================================

    /** 显示错误信息（覆盖所有页面） */
    void showError(const char* title, const char* message);

    /** 清屏 */
    void clear();

    /** 屏幕是否初始化成功 */
    bool isOK();

private:
    Adafruit_SSD1306 _display;   // SSD1306 对象
    bool _initialized;
    uint8_t _currentPage;
    unsigned long _lastUpdateTime;

    // 三个页面的绘制函数
    void _drawMainPage(const SensorData& data, bool relayOn, const char* statusText);
    void _drawNetworkPage();
    void _drawSystemPage();

    // 工具函数
    void _drawPageIndicator();    // 底部圆点指示器
    void _drawHeader(const char* title);  // 标题栏（反白）
    void _drawDivider(int y);     // 水平分割线
};

#endif // DISPLAY_MANAGER_H
