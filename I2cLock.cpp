#include "I2cLock.h"
#include <Arduino.h>
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t g_i2cMux;

void i2cLockInit() {
    if (g_i2cMux == nullptr) {
        g_i2cMux = xSemaphoreCreateRecursiveMutex();
    }
}

void i2cLock() {
    if (g_i2cMux != nullptr) {
        xSemaphoreTakeRecursive(g_i2cMux, portMAX_DELAY);
    }
}

void i2cUnlock() {
    if (g_i2cMux != nullptr) {
        xSemaphoreGiveRecursive(g_i2cMux);
    }
}

void i2cBusRecovery(int sdaPin, int sclPin) {
    // 將 SCL 切為 GPIO output，SDA 切為 input（帶上拉）
    pinMode(sclPin, OUTPUT);
    pinMode(sdaPin, INPUT_PULLUP);
    digitalWrite(sclPin, HIGH);
    delayMicroseconds(10);

    // 送最多 9 個 clock 脈衝，讓卡住的 slave 釋放 SDA
    for (int i = 0; i < 9; i++) {
        if (digitalRead(sdaPin)) break;   // SDA 已高（空閒），可提前結束
        digitalWrite(sclPin, LOW);
        delayMicroseconds(5);
        digitalWrite(sclPin, HIGH);
        delayMicroseconds(5);
    }

    // 產生 STOP condition：SCL 高時 SDA 由低→高
    pinMode(sdaPin, OUTPUT);
    digitalWrite(sdaPin, LOW);
    delayMicroseconds(5);
    digitalWrite(sclPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(sdaPin, HIGH);
    delayMicroseconds(5);

    // 恢復硬體 I2C
    Wire.begin(sdaPin, sclPin);
    Wire.setClock(400000);
}
