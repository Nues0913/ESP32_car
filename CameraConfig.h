#ifndef CAMERA_CONFIG_H
#define CAMERA_CONFIG_H

#include "esp_camera.h"
#include "pinConfig.h"
#include <WiFiUdp.h>

#define UDP_CAM_MAGIC          0xCAFEFACE
#define UDP_CAM_MAX_PAYLOAD    1400
#define UDP_CHUNK_GAP_US       0
#define CAMERA_TIMESHIFT_US    0

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;        // sync word
    uint64_t timestamp_us; // esp time
    uint32_t frame_id;
    uint32_t total_size;
    uint16_t chunk_id;
    uint16_t total_chunks;
    uint16_t payload_size;
} udp_header_t;
#pragma pack(pop)

void initCamera();
void startCameraUDP();

#endif
