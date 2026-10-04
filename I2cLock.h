#ifndef I2C_LOCK_H
#define I2C_LOCK_H

// 同一條 Wire 上多裝置（PCA9685 + BMI088）與多任務並存時必須序列化存取。
// 使用遞迴 mutex：initMotor() 內呼叫 carStop() 等不會自鎖。
void i2cLockInit();
void i2cLock();
void i2cUnlock();

// I2C 匯流排復原：以 bit-bang 方式送 9 個 SCL 脈衝 + STOP，釋放被
// slave 拉住的 SDA（常見於 ESP32 重啟後 BMI088 卡住 I2C 的情境）。
// 呼叫後會自動重新 Wire.begin()，恢復硬體 I2C 功能。
void i2cBusRecovery(int sdaPin, int sclPin);

#endif
