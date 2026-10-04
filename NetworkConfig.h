#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <WiFi.h>
#include <WiFiUdp.h>

// Wi-Fi settings
extern const char *WIFI_SSID;
extern const char *WIFI_PSK;
extern const uint16_t UDP_PORT;
extern const uint16_t UDP_IMU_PORT;
extern const uint16_t UDP_DEBUG_PORT;
extern const uint16_t UDP_CAM_PORT;
extern IPAddress ALLOW_SRC;
extern IPAddress DEBUG_IP;
extern IPAddress IMU_IP;


// UDP
extern WiFiUDP UdpDeb;     // debug 輸出
extern WiFiUDP UdpCmd;     // 控制指令接收
extern WiFiUDP UdpIMU;     // IMU 資料廣播
extern WiFiUDP UdpCam;     // Camera 串流

// Timeout
extern unsigned long lastPacketMs;
extern const unsigned long TIMEOUT_MS;

// Function declarations
void initNetwork();

#endif
