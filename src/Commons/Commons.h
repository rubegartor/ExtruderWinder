#pragma once

#include <Arduino.h>
#include <EEPROM_Preferences.h>
#include "Motor/Aligner/Aligner.h"
#include "Motor/Puller/Puller.h"
#include "Motor/Spooler/Spooler.h"
#include "Tensioner/Tensioner.h"
#include "Measurement/Measurement.h"

#define TMC5160_CLK 12

#define STEPPER_DEFAULT_STEPS 200

#define DIAMETER_PREF "diameter"
#define AUTOSTOP_PREF "autostop"

#define DIAMETER_DEFAULT 1.75f
#define AUTOSTOP_DEFAULT 0.2f

extern Aligner aligner;
extern Puller puller;
extern Spooler spooler;
extern Tensioner tensioner;
extern Measurement measurement;
extern EEPROM_Preferences preferences;

extern float diameter;
extern float autostop;

extern int32_t to_move;

void initCommons();

void updateDiameter(float value);
void updateAutostop(float value);

char* getTime(unsigned long millis);

float getExtrudedLength();
float getExtrudedWeight();