#pragma once

#include <Arduino.h>

#define SPOOL_DIR_PIN 33
#define SPOOL_STEP_PIN 32
#define SPOOL_CS_PIN 15
#define SPOOL_MAX_SPEED 10000

void winder(void* pvParameters);