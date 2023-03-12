#pragma once

#include <Arduino.h>

#define SPOOL_DIR_PIN 33
#define SPOOL_STEP_PIN 32
#define SPOOL_CS_PIN 15
#define SPOOL_MAX_SPEED 10000

#define PULLER_DIR_PIN 14 // Not connected
#define PULLER_STEP_PIN 25
#define PULLER_MAX_SPEED 13200
#define PULLER_DIAM 24.5 //In mm

#define MIN_DISTANCE 55
#define OFFSET_DISTANCE 100

void IRAM_ATTR wTask(void* pvParameters);

float getExtrudedLength();