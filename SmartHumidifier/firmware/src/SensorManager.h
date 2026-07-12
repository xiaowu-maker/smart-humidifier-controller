/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * SensorManager - 温湿度传感器管理模块（头文件）
 * ============================================================
 *
 * 职责：管理 AHT20 温湿度传感器的一切操作。
 *       其他模块不需要知道传感器的型号和通信细节，
 *       只需要调用 getTemperature() 和 getHumidity()。
 *
 * 传感器：AHT20（I2C 地址 0x38）
 * 精度：  ±0.3℃（温度）/ ±2%（湿度）
 * 速度：  每次测量约 80ms
 *
 * 设计要点：
 *   1. 传感器故障时返回上次有效值（不崩溃）
 *   2. 数据异常时自动过滤（范围检查）
 *   3. 连续失败达到阈值时报警
 *   4. 支持温度/湿度偏移校准
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Adafruit_AHTX0.h>

/**
 * 传感器数据结构体
 * 统一的数据格式，所有模块间传递这个结构体
 */
struct SensorData {
    float temperature;      // 温度（摄氏度）
    float humidity;         // 相对湿度（%RH，0-100）
    bool valid;             // 本次数据是否有效
    unsigned long timestamp; // 采集时刻（millis）
};

class SensorManager {
public:
    // ============================================================
    // 生命周期
    // ============================================================

    /** 初始化传感器，返回 true=成功 */
    bool begin();

    /**
     * 执行一次完整的温度+湿度读取
     * 内部自动判断读取间隔，无需每次调用都真正读硬件
     * 返回 true=本次读取成功
     */
    bool readSensor();

    // ============================================================
    // 数据获取（不触发硬件读取，只返回缓存值）
    // ============================================================

    /** 获取最新温度（℃），保留 1 位小数 */
    float getTemperature();

    /** 获取最新湿度（%RH），保留 1 位小数 */
    float getHumidity();

    /** 获取完整传感器数据 */
    SensorData getData();

    // ============================================================
    // 状态查询
    // ============================================================

    /** 传感器是否正常工作 */
    bool isOK();

    /** 获取错误描述（最近一次错误的文字说明） */
    const char* getLastError();

    /** 连续失败次数 */
    int getConsecutiveErrors();

    /** 总读取次数（调试用） */
    unsigned long getReadCount();

    /** 总失败次数（调试用） */
    unsigned long getErrorCount();

    // ============================================================
    // 校准（如果跟标准仪器对比有偏差，可以微调）
    // ============================================================

    /** 设置温度偏移量，正值=显示偏高 */
    void setTemperatureOffset(float offset);

    /** 设置湿度偏移量 */
    void setHumidityOffset(float offset);

    // ============================================================
    // 诊断
    // ============================================================

    /** 尝试软复位传感器 */
    void resetSensor();

private:
    Adafruit_AHTX0 _aht;             // Adafruit 库的 AHT20 对象
    bool _initialized;               // 传感器是否初始化成功
    bool _dataValid;                 // 最后一次读数是否有效

    // 缓存数据（传感器故障时返回这些值）
    float _temperature;
    float _humidity;
    unsigned long _lastReadTime;

    // 校准偏移
    float _tempOffset;
    float _humidityOffset;

    // 错误统计
    int _consecutiveErrors;
    unsigned long _totalReads;
    unsigned long _totalErrors;
    const char* _lastError;

    // ============================================================
    // 内部方法
    // ============================================================

    /** 验证数据是否在合理范围内 */
    bool _validateData(float temp, float humidity);

    /** 检查距离上次读取是否已过足够时间 */
    bool _shouldRead();
};

#endif // SENSOR_MANAGER_H
