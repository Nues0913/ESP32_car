#ifndef DEBUG_CONFIG_H
#define DEBUG_CONFIG_H

#include <Arduino.h>
#include "NetworkConfig.h"

// Function declarations
void debugPrint(const char* msg);
void debugPrintln(const char* msg);
void debugPrintf(const char* format, ...);

#endif
