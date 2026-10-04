#include "IMUControl.h"
#include "NetworkConfig.h"
#include "DebugConfig.h"
#include "I2cLock.h"
#include "pinConfig.h"

#include <Wire.h>
#include <math.h>
#include <esp_timer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <BMI088.h>

// 200Hz = 5ms
static constexpr uint32_t kImuPeriodMs = 5;

// Bmi088 Data Synchronization
// 硬體須將 Gyro INT3 短接至 Accel INT1
static Bmi088 *g_bmi088 = nullptr;

static uint32_t s_consecErrors   = 0;
static uint32_t s_reInitAttempts = 0;
static uint64_t s_lastReInitUs   = 0;
static constexpr uint32_t kBadReadThreshold  = 5;        // 連續壞讀 N 次觸發 re-init
static constexpr uint64_t kReInitCooldownUs  = 2000000;  // re-init 冷卻 2 秒
static constexpr uint32_t kMaxReInitAttempts = 5;         // 放棄前最多嘗試次數

// 內部重新初始化
static void reinitIMU() {
    debugPrintln("[IMU] attempting re-init with bus recovery...");

    i2cLock();
    delete g_bmi088; g_bmi088 = nullptr;
    i2cBusRecovery(I2C_SDA, I2C_SCL);
    i2cUnlock();

    if (initIMU()) {
        debugPrintln("[IMU] re-init SUCCESS");
    } else {
        debugPrintln("[IMU] re-init FAILED");
    }
}

bool initIMU() {
    if (g_bmi088) {
        return true;
    }

    debugPrintln("[IMU] scanning BMI088 (data-sync, INT3→INT1 required)...");

    static const uint8_t kAccelAddrs[] = {0x18, 0x19};
    static const uint8_t kGyroAddrs[]  = {0x68, 0x69};

    for (uint8_t a : kAccelAddrs) {
        for (uint8_t g : kGyroAddrs) {
            debugPrintf("[IMU] try accel 0x%02X + gyro 0x%02X\n",
                        (unsigned)a, (unsigned)g);
            i2cLock();

            Bmi088 *imu = new Bmi088(Wire, a, g);
            int st = imu->begin();
            if (st < 0) {
                delete imu;
                i2cUnlock();
                debugPrintf("[IMU]   begin() failed (st=%d), skip\n", st);
                continue;
            }
            if (!imu->setRange(Bmi088::ACCEL_RANGE_6G, Bmi088::GYRO_RANGE_2000DPS)) {
                delete imu;
                i2cUnlock();
                debugPrintln("[IMU]   setRange failed, skip");
                continue;
            }
            if (!imu->setOdr(Bmi088::ODR_400HZ)) {
                delete imu;
                i2cUnlock();
                debugPrintln("[IMU]   setOdr failed, skip");
                continue;
            }

            g_bmi088 = imu;
            i2cUnlock();
            debugPrintf(
                "[IMU] OK: accel 0x%02X, gyro 0x%02X (data-sync)\n",
                (unsigned)a, (unsigned)g);
            return true;
        }
    }
    debugPrintln("[IMU] scan done: no working BMI088 address pair");
    return false;
}

bool readIMU(IMUData &data) {
    if (!g_bmi088) {
        data.valid = false;
        return false;
    }

    // 讀取感測器
    i2cLock();
    g_bmi088->readSensor();
    data.ax = g_bmi088->getAccelX_mss();
    data.ay = g_bmi088->getAccelY_mss();
    data.az = g_bmi088->getAccelZ_mss();
    data.gx = g_bmi088->getGyroX_rads();
    data.gy = g_bmi088->getGyroY_rads();
    data.gz = g_bmi088->getGyroZ_rads();
    i2cUnlock();

    // 資料驗證
    bool dataOk = true;

    // 加速度計 3 軸完全相同
    if (data.ax == data.ay && data.ay == data.az) {
        dataOk = false;
    }

    // 非有限數值檢測（NaN,Infinity）
    if (!isfinite(data.ax) || !isfinite(data.ay) || !isfinite(data.az) ||
        !isfinite(data.gx) || !isfinite(data.gy) || !isfinite(data.gz)) {
        dataOk = false;
    }

    if (!dataOk) {
        s_consecErrors++;
        data.valid = false;

        // 連續壞讀 re-init
        if (s_consecErrors >= kBadReadThreshold &&
            s_reInitAttempts < kMaxReInitAttempts) {
            uint64_t nowUs = esp_timer_get_time();
            if ((nowUs - s_lastReInitUs) > kReInitCooldownUs) {
                s_lastReInitUs = nowUs;
                s_reInitAttempts++;
                s_consecErrors = 0;
                debugPrintf("[IMU] re-init attempt %u/%u\n",
                            (unsigned)s_reInitAttempts,
                            (unsigned)kMaxReInitAttempts);
                reinitIMU();
            }
        }
        return false;
    }

    // 資料正常
    s_consecErrors   = 0;
    s_reInitAttempts = 0;
    data.valid = true;
    return true;
}

static void imuUdpStreamTask(void * /*pvParameters*/) {
    IMUData data;
    const TickType_t period = pdMS_TO_TICKS(kImuPeriodMs);
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&lastWake, period);
        uint64_t timestamp = esp_timer_get_time();  // 取於讀取前，與 fb->timestamp 同源時鐘

        if (!readIMU(data) || !data.valid) {
            continue;
        }

        static uint32_t s_seq = 0;
        ImuPacket pkt;
        pkt.timestamp = timestamp;
        pkt.ax = data.ax;
        pkt.ay = data.ay;
        pkt.az = data.az;
        pkt.gx = data.gx;
        pkt.gy = data.gy;
        pkt.gz = data.gz;
        pkt.seq = s_seq++;

        UdpIMU.beginPacket(IMU_IP, UDP_IMU_PORT);
        UdpIMU.write((uint8_t *)&pkt, sizeof(ImuPacket));
        UdpIMU.endPacket();
    }
}

void startImuUdpStreamTask() {
    static bool started = false;
    if (started) {
        return;
    }
    started = true;
    // 固定週期與 loop 分離；pin 到 Core 1，減少與 WiFi 同核競態
    constexpr UBaseType_t kPriority = 5;
    constexpr uint32_t kStackWords = 4096;
    xTaskCreatePinnedToCore(
        imuUdpStreamTask,
        "imuUdp",
        kStackWords,
        nullptr,
        kPriority,
        nullptr,
        1);
}
