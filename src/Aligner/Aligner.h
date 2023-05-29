#pragma once

#include <Arduino.h>

#define ALIGNER_DIR_PIN 5
#define ALIGNER_STEP_PIN 4
#define ALIGNER_CS_PIN 2
#define ALIGNER_HOME_SENSOR_PIN 39  // NO SE USA (ANTIGUO SENSOR HOME)
#define ALIGNER_FIRST_MOVE -300
#define ALIGNER_MAX_SPEED 10000

// Número de lecturas de stallguard a ignorar
#define STALLGUARD_IGNORE 10

// Si el stall value es menor a este número se considera el stall
#define STALLGUARD_THRESHOLD 80

void configAlignerDriver();

void aTask(void* pvParameters);

void startAlignerPosition();

void resetHome();

void resetSpoolerRevs();

bool isPositioned();

bool isHomed();