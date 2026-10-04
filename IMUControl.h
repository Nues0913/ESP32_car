#ifndef IMU_CONTROL_H
#define IMU_CONTROL_H

#include <Arduino.h>

// BMI088 I2C 位址由 IMUControl.cpp 於啟動時掃描 0x18/0x19 與 0x68/0x69

// Binary packet structure for ORB-SLAM3 (36 bytes total)
// Format compatible with ROS sensor_msgs/Imu standard
struct ImuPacket {
    uint64_t timestamp;  // 微秒 (μs) - 8 bytes
    float ax, ay, az;    // 加速度 (m/s²) - 12 bytes
    float gx, gy, gz;    // 角速度 (rad/s) - 12 bytes
    uint32_t seq;        // 封包序號 - 4 bytes
} __attribute__((packed));

// IMU data（BMI088 SI：m/s²、rad/s）
struct IMUData {
    float ax, ay, az;
    float gx, gy, gz;
    bool valid;
};

// Function declarations
bool initIMU();
bool readIMU(IMUData &data);
// 於獨立 FreeRTOS 任務內以固定 200Hz 送 IMU UDP（在 initIMU() 成功後呼叫一次）
void startImuUdpStreamTask();

#endif
