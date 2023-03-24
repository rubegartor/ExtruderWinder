#pragma once

#include <Arduino.h>

#define ALIGNER_DIR_PIN 5
#define ALIGNER_STEP_PIN 4
#define ALIGNER_CS_PIN 2
#define ALIGNER_HOME_SENSOR_PIN 39
#define ALIGNER_FIRST_MOVE -300
#define ALIGNER_MAX_SPEED 10000

void aTask(void* pvParameters);

void resetSpoolerRevs();

int16_t moveStep();