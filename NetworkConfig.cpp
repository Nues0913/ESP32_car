#include "NetworkConfig.h"
#include "esp_wifi.h"

const char *WIFI_SSID = "XXXXXX";
const char *WIFI_PSK  = "XXXXXX";
const uint16_t UDP_PORT = 4000;
const uint16_t UDP_IMU_PORT = 4001;
const uint16_t UDP_DEBUG_PORT = 4002;
const uint16_t UDP_CAM_PORT = 4003;

IPAddress ALLOW_SRC(0, 0, 0, 0);
IPAddress DEBUG_IP(255, 255, 255, 255);
IPAddress IMU_IP(255, 255, 255, 255);
WiFiUDP UdpDeb;     // debug 輸出
WiFiUDP UdpCmd;     // 控制指令接收
WiFiUDP UdpIMU;     // IMU 資料廣播
WiFiUDP UdpCam;     // Camera 串流

// Timeout variables
unsigned long lastPacketMs = 0;
const unsigned long TIMEOUT_MS = 300;

void initNetwork() {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname("esp-car");
    WiFi.begin(WIFI_SSID, WIFI_PSK);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }
    esp_wifi_set_ps(WIFI_PS_NONE);  // 關閉省電，降低 UDP 延遲尖峰

    // 計算子網廣播地址
    IPAddress localIP = WiFi.localIP();
    IPAddress broadcastIP(localIP[0], localIP[1], localIP[2], 255);
    IMU_IP = broadcastIP;
    DEBUG_IP = broadcastIP;
    UdpDeb.begin(UDP_DEBUG_PORT);
    UdpCmd.begin(UDP_PORT);
    UdpIMU.begin(UDP_IMU_PORT);
    UdpCam.begin(UDP_CAM_PORT);
}
