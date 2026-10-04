#include "DebugConfig.h"
#include <stdarg.h>


void debugPrint(const char* msg) {
    if (WiFi.status() == WL_CONNECTED) {
        UdpDeb.beginPacket(DEBUG_IP, UDP_DEBUG_PORT);
        UdpDeb.print(msg);
        UdpDeb.endPacket();
    }
}

void debugPrintln(const char* msg) {
    if (WiFi.status() == WL_CONNECTED) {
        UdpDeb.beginPacket(DEBUG_IP, UDP_DEBUG_PORT);
        UdpDeb.print(msg);
        UdpDeb.print("\n");
        UdpDeb.endPacket();
    }
}

void debugPrintf(const char* format, ...) {
    if (WiFi.status() == WL_CONNECTED) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        UdpDeb.beginPacket(DEBUG_IP, UDP_DEBUG_PORT);
        UdpDeb.print(buffer);
        UdpDeb.endPacket();
    }
}
