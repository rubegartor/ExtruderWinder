#pragma once

#include <Arduino.h>

#define SPOOL_STEP_PIN 32
#define SPOOL_CS_PIN 15
#define SPOOL_MAX_SPEED 4000

#define PULLER_STEP_PIN 25
#define PULLER_MAX_SPEED 13200
#define PULLER_DIAM 24.5f  // In mm

void configSpoolDriver();

void IRAM_ATTR wTask(void* pvParameters);

void toggleAlignerTestRun();

float getExtrudedLength();

float getExtrudedWeight();