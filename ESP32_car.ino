#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Wire.h>
#include "pinConfig.h"
#include "CameraConfig.h"
#include "MotorControl.h"
#include "NetworkConfig.h"
#include "IMUControl.h"
#include "I2cLock.h"
#include "DebugConfig.h"

void setup()
{
    // Brownout disable
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    debugPrintln("System initializing...");

    // I2C 匯流排復原：若上次斷電 / reset 時 BMI088 正在傳輸，SDA 可能被拉低，
    // 導致 Wire.begin() 後整條 I2C 都不通。先用 bit-bang 送 clock 釋放。
    i2cLockInit();         // PCA9685 與 BMI088 共用 Wire，避免多任務互搶匯流排
    i2cBusRecovery(I2C_SDA, I2C_SCL);   // 內含 Wire.begin + setClock(400k)
    debugPrintln("[OK] I2C initialized (with bus recovery)");

    initMotor();
    debugPrintln("[OK] Motor initialized");

    initNetwork();

    debugPrintf("[OK] WiFi connected: %s\n", WiFi.localIP().toString().c_str());
    debugPrintf("[OK] UDP Port: %d (control), %d (IMU)\n", UDP_PORT, UDP_IMU_PORT);

    initCamera();
    startCameraUDP();
    debugPrintf("[OK] Camera UDP stream on port %d\n", UDP_CAM_PORT);

    // 初始化 IMU，並在獨立任務內固定 200Hz 送 UDP
    // if (initIMU()) {
    //     startImuUdpStreamTask();
    //     debugPrintln("[OK] IMU initialized (200Hz UDP task)");
    // } else {
    //     debugPrintln("[WARN] IMU not found");
    // }

    debugPrintln("=== System Ready ===\n");
}

void loop()
{
    // 優先處理 UDP 控制指令（確保控制響應及時）
    int packetSize = UdpCmd.parsePacket();
    if (packetSize > 0)
    {
        // 檢查來源（可選）
        if (ALLOW_SRC != IPAddress(0, 0, 0, 0) && UdpCmd.remoteIP() != ALLOW_SRC)
        {
            while (UdpCmd.available()) UdpCmd.read();
        }
        else
        {
            char buf[64] = {0};
            int len = UdpCmd.read(buf, sizeof(buf) - 1);
            if (len > 0)
            {
                buf[len] = '\0';
                // Serial.printf("%s\n", buf);
                parseAndApply(buf);
                lastPacketMs = millis();
            }
        }
    }

    // 超時保護
    if (millis() - lastPacketMs > TIMEOUT_MS)
    {
        carStop();
    }
}
 