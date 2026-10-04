#include "CameraConfig.h"
#include "NetworkConfig.h"
#include "DebugConfig.h"
#include <string.h>

static uint32_t  g_frame_id = 0;
static IPAddress g_client_ip;
static uint16_t  g_client_port = 0;

static void cameraUDPTask(void *pvParameters) {
    uint8_t packet_buf[sizeof(udp_header_t) + UDP_CAM_MAX_PAYLOAD];

    while (true) {
        int pkt = UdpCam.parsePacket();
        if (pkt > 0) {
            char cmd[8] = {0};
            int len = UdpCam.read(cmd, sizeof(cmd) - 1);
            if (len >= 3 && memcmp(cmd, "SUB", 3) == 0) {
                g_client_ip   = UdpCam.remoteIP();
                g_client_port = UdpCam.remotePort();
                IMU_IP = g_client_ip;   // IMU 切換 unicast
            }
        }

        // no client
        if (g_client_port == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) continue;

        int64_t ts_us = (int64_t)fb->timestamp.tv_sec * 1000000LL
                      + (int64_t)fb->timestamp.tv_usec;
        uint64_t timestamp = (uint64_t)(ts_us + CAMERA_TIMESHIFT_US);
        uint32_t frame_id   = g_frame_id++;
        uint32_t total_size  = fb->len;
        uint16_t total_chunks = (total_size + UDP_CAM_MAX_PAYLOAD - 1) / UDP_CAM_MAX_PAYLOAD;

        for (uint16_t i = 0; i < total_chunks; i++) {
            uint32_t offset = (uint32_t)i * UDP_CAM_MAX_PAYLOAD;
            uint16_t payload_size = (i == total_chunks - 1)
                ? (uint16_t)(total_size - offset)
                : UDP_CAM_MAX_PAYLOAD;

            udp_header_t *hdr = (udp_header_t *)packet_buf;
            hdr->magic        = UDP_CAM_MAGIC;
            hdr->timestamp_us = timestamp;
            hdr->frame_id     = frame_id;
            hdr->total_size   = total_size;
            hdr->chunk_id     = i;
            hdr->total_chunks = total_chunks;
            hdr->payload_size = payload_size;

            memcpy(packet_buf + sizeof(udp_header_t), fb->buf + offset, payload_size);

            UdpCam.beginPacket(g_client_ip, g_client_port);
            UdpCam.write(packet_buf, sizeof(udp_header_t) + payload_size);
            UdpCam.endPacket();

            if (i < total_chunks - 1) {
                delayMicroseconds(UDP_CHUNK_GAP_US);
            }
        }

        esp_camera_fb_return(fb);
    }
}

void initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = Y2_GPIO_NUM;
    config.pin_d1       = Y3_GPIO_NUM;
    config.pin_d2       = Y4_GPIO_NUM;
    config.pin_d3       = Y5_GPIO_NUM;
    config.pin_d4       = Y6_GPIO_NUM;
    config.pin_d5       = Y7_GPIO_NUM;
    config.pin_d6       = Y8_GPIO_NUM;
    config.pin_d7       = Y9_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;

    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size   = FRAMESIZE_VGA;  // 640×480
    config.jpeg_quality = 15;
    config.fb_count     = 2;
    config.grab_mode    = CAMERA_GRAB_LATEST;

    if (esp_camera_init(&config) != ESP_OK) {
        while (true) delay(100);
    }
    // sensor_t *s = esp_camera_sensor_get();

    // // --- 曝光與增益 ---
    // s->set_exposure_ctrl(s, 0);
    // s->set_aec_value(s, 200);   // 稍微拉高一點點曝光，減少因畫面過暗產生的底噪
    // s->set_gain_ctrl(s, 0);
    // s->set_agc_gain(s, 2);      // 將增益壓到極低 (0~3之間)，這是顆粒雜訊的主要來源

    // // --- 影像基礎屬性 (退回平滑設定) ---
    // s->set_contrast(s, 0);      // 歸零，停止放大雜訊
    // s->set_saturation(s, -2);   // 保持低飽和度，降低彩色雜訊干擾
    // s->set_sharpness(s, 0);     // 歸零，銳化也會讓雜訊邊緣更明顯

    // // --- ★ 開啟 OV2640 內建的 DSP 降噪與校正功能 ---
    // s->set_bpc(s, 1);           // 開啟黑點補償 (Black Pixel Correction)，消除暗部雜訊
    // s->set_wpc(s, 1);           // 開啟白點補償 (White Pixel Correction)，消除過熱亮點
    // s->set_raw_gma(s, 1);       // 開啟 Gamma 校正，讓亮暗過渡更平滑
    // s->set_lenc(s, 1);          // 開啟鏡頭邊緣校正 (Lens Correction)，減少
}

void startCameraUDP() {
    xTaskCreatePinnedToCore(
        cameraUDPTask,
        "cam_udp",
        8192,
        NULL,
        1,
        NULL,
        0       // Core 0（與 WiFi 同核，避免跨核 lwIP 鎖競爭）
    );
}
