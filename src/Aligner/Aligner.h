#pragma once

#include <Arduino.h>

#define ALIGNER_DIR_PIN 5
#define ALIGNER_STEP_PIN 4
#define ALIGNER_CS_PIN 2
#define ALIGNER_STARTUP_MOVE -100
#define ALIGNER_MAX_SPEED 10000

#define ALIGNER_MAX_DISTANCE -5300 // En steps

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