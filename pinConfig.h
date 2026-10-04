#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// ===== LED =====
#define LED_PIN 4

// ===== I2C 腳位 =====
#define I2C_SDA 1
#define I2C_SCL 3
//
// BMI088 常見 breakout 對照：
//   VCC/GND => 3.3V 與 GND
//   SDA/SCL => I2C_SDA / I2C_SCL
//   AD0     => 加速度計 I2C 位址低位（接 GND 多為 0x18，接 VCC 多為 0x19）
//   INT1    => 接 Gyro INT3，供目前的資料同步模式使用
//   INT3    => 接 Accel INT1
//   NC      => 空腳

// ===== Camera 腳位 =====
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#endif
