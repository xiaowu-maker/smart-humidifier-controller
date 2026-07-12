/**
 * ============================================================
 * Smart Humidifier Controller (SHC) V1.0
 * SensorManager - 温湿度传感器管理模块（实现文件）
 * ============================================================
 */

#include "SensorManager.h"
#include "config.h"
#include "Logger.h"

// ============================================================
// 初始化
// ============================================================

bool SensorManager::begin() {
    LOG_INFO("SENSOR", "Initializing AHT20 sensor...");

    _initialized = false;
    _dataValid = false;
    _consecutiveErrors = 0;
    _totalReads = 0;
    _totalErrors = 0;
    _tempOffset = 0.0f;
    _humidityOffset = 0.0f;
    _lastError = "";
    _temperature = 0.0f;
    _humidity = 0.0f;
    _lastReadTime = 0;

    // Adafruit_AHTX0 库内部会自动调用 Wire.begin()
    // begin() 返回 true 表示找到了 AHT20 设备
    if (!_aht.begin()) {
        _initialized = false;
        _lastError = "AHT20 not found at I2C address 0x38";
        LOG_ERROR("SENSOR", _lastError);
        LOG_ERROR("SENSOR", "Check: VCC=3.3V, SDA=GPIO21, SCL=GPIO22");
        return false;
    }

    _initialized = true;
    LOG_INFO("SENSOR", "AHT20 sensor initialized OK");
    return true;
}

// ============================================================
// 读取传感器
// ============================================================

bool SensorManager::readSensor() {
    // 1. 检查传感器是否已初始化
    if (!_initialized) {
        _lastError = "Sensor not initialized";
        _consecutiveErrors++;
        _totalErrors++;
        return false;
    }

    // 2. 检查是否需要读取（避免频繁读取）
    if (!_shouldRead()) {
        return _dataValid;
    }

    // 3. 执行读取
    sensors_event_t humidityEvent, tempEvent;

    // getEvent 返回 false 表示读取失败
    if (!_aht.getEvent(&humidityEvent, &tempEvent)) {
        _consecutiveErrors++;
        _totalErrors++;
        _lastError = "I2C read failed";

        // 第一次失败只是警告
        if (_consecutiveErrors == 1) {
            LOG_WARN("SENSOR", "Read failed (1st attempt), retrying next cycle...");
        }

        // 连续失败达到阈值
        if (_consecutiveErrors >= SENSOR_MAX_ERRORS) {
            LOG_ERROR("SENSOR", "Sensor disconnected! 3 consecutive failures.");
            LOG_ERROR("SENSOR", "System will use last known values.");
        }

        _dataValid = false;
        return false;
    }

    // 4. 提取数值
    float rawTemp = tempEvent.temperature;      // 原始温度
    float rawHumidity = humidityEvent.relative_humidity; // 原始湿度

    // 5. 数据验证
    if (!_validateData(rawTemp, rawHumidity)) {
        _consecutiveErrors++;
        _totalErrors++;
        _lastError = "Data out of valid range";
        LOG_WARN("SENSOR", "Invalid data: ignoring this reading");
        _dataValid = false;
        return false;
    }

    // 6. 应用校准偏移
    _temperature = rawTemp + _tempOffset;
    _humidity = rawHumidity + _humidityOffset;

    // 7. 更新状态
    _dataValid = true;
    _consecutiveErrors = 0;
    _totalReads++;
    _lastReadTime = millis();

    // 调试输出（每 60 次读取输出一次统计）
    if (_totalReads % 60 == 0) {
        float errorRate = (_totalErrors * 100.0f) / (_totalReads + _totalErrors);
        LOG_DEBUG("SENSOR",
            ("Stats: " + String(_totalReads) + " reads, " +
             String(_totalErrors) + " errors (" +
             String(errorRate, 1) + "%)").c_str());
    }

    return true;
}

// ============================================================
// 数据获取
// ============================================================

float SensorManager::getTemperature() {
    return _temperature;
}

float SensorManager::getHumidity() {
    return _humidity;
}

SensorData SensorManager::getData() {
    SensorData data;
    data.temperature = _temperature;
    data.humidity = _humidity;
    data.valid = _dataValid;
    data.timestamp = _lastReadTime;
    return data;
}

// ============================================================
// 状态查询
// ============================================================

bool SensorManager::isOK() {
    return _initialized && (_consecutiveErrors < SENSOR_MAX_ERRORS);
}

const char* SensorManager::getLastError() {
    return _lastError;
}

int SensorManager::getConsecutiveErrors() {
    return _consecutiveErrors;
}

unsigned long SensorManager::getReadCount() {
    return _totalReads;
}

unsigned long SensorManager::getErrorCount() {
    return _totalErrors;
}

// ============================================================
// 校准
// ============================================================

void SensorManager::setTemperatureOffset(float offset) {
    _tempOffset = offset;
    LOG_INFO("SENSOR", ("Temperature offset set to " + String(offset, 1) + "°C").c_str());
}

void SensorManager::setHumidityOffset(float offset) {
    _humidityOffset = offset;
    LOG_INFO("SENSOR", ("Humidity offset set to " + String(offset, 1) + "%").c_str());
}

// ============================================================
// 诊断
// ============================================================

void SensorManager::resetSensor() {
    LOG_INFO("SENSOR", "Attempting sensor reset...");

    // Adafruit_AHTX0 库没有直接的 reset 方法
    // 通过重新调用 begin() 来软复位
    if (_aht.begin()) {
        _consecutiveErrors = 0;
        _dataValid = false;
        LOG_INFO("SENSOR", "Sensor reset OK");
    } else {
        LOG_ERROR("SENSOR", "Sensor reset FAILED");
    }
}

// ============================================================
// 内部方法
// ============================================================

bool SensorManager::_validateData(float temp, float humidity) {
    // AHT20 规格范围：
    //   温度：-40℃ ~ +85℃
    //   湿度：0% ~ 100%
    // 我们收窄一点，过滤明显的异常值

    if (temp < -40.0f || temp > 85.0f) {
        LOG_WARN("SENSOR", ("Temperature out of range: " + String(temp, 1) + "°C").c_str());
        return false;
    }

    if (humidity < 0.0f || humidity > 100.0f) {
        LOG_WARN("SENSOR", ("Humidity out of range: " + String(humidity, 1) + "%").c_str());
        return false;
    }

    // 变化率检查：两次读数之间波动不能太大
    // （如果你有上次有效值的话）
    if (_dataValid) {
        float tempDelta = abs(temp - _temperature);
        float humDelta = abs(humidity - _humidity);

        // 2秒内温度变化超过 10℃ 不太可能（除非传感器坏了）
        if (tempDelta > 10.0f) {
            LOG_WARN("SENSOR", "Temperature spike detected! Ignoring.");
            return false;
        }

        // 2秒内湿度变化超过 20% 也不太可能
        if (humDelta > 20.0f) {
            LOG_WARN("SENSOR", "Humidity spike detected! Ignoring.");
            return false;
        }
    }

    return true;
}

bool SensorManager::_shouldRead() {
    // 第一次读取不允许跳过
    if (_lastReadTime == 0) return true;

    // 距离上次读取超过设定间隔才允许再次读取
    return (millis() - _lastReadTime >= SENSOR_READ_INTERVAL_MS);
}
