#pragma once

#include <AiEsp32RotaryEncoder.h>
#include <Arduino.h>
#include <Commons/Commons.h>
#include <LCD/LCDMenu.h>
#include <Measuring/Measuring.h>
#include <PID/PIDPuller.h>
#include <PID/PIDSpooler.h>
#include <Preferences.h>
#include <RotaryEncoder/RotaryEncoder.h>
#include <Tensioner/Tensioner.h>
#include <WiFi/WifiOut.h>

#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"

#define BUZZER_PIN 12

#define STEPPER_DEF_STEPS 200
#define DEFAULT_PULLER_SPEED 1500
#define NAMESPACE "extruder"

#define FILAMENT_DIAMETER_MODE_PREF "filaDiam"
#define DEFAULT_FILAMENT_DIAMETER 1.75f

#define SELECTED_POLYMER_PREF "selPolymer"
#define POLYMER_NUMBER 8

const float spoolMotorRatio = 5.16;
const uint16_t stepsPerCm = 525;
const uint16_t oneRevSpool = (STEPPER_DEF_STEPS * 4) * spoolMotorRatio;
const uint16_t oneRevPuller = STEPPER_DEF_STEPS * 16;

extern Preferences pref;
extern WifiOut wifiOut;
extern PIDPuller pidPuller;
extern PIDSpooler pidSpooler;
extern Measuring measuring;
extern LCDMenu lcdMenu;
extern Tensioner tensioner;
extern REncoder rotaryEncoder;

// Global variables
struct Polymer {
  String name;
  float weight;          // Gramos por metro
  float diameterOffset;  // La cantidad de diametro que hay que obviar (ej, En
                         // los filamentos flexibles el sensor mide menos que el
                         // diametro final, por lo tanto se restará este offset
                         // para calcularlo)
};

extern bool homed;
extern bool needHome;
extern uint16_t spoolTotalRevs;
extern uint16_t pullerTotalRevs;
extern uint16_t spoolSpeed;
extern uint16_t pullerSpeed;
extern float filamentDiameter;
extern ulong millisOffset;
extern Polymer polymers[POLYMER_NUMBER];

void commonsInit();

bool isHomed();

bool isReady();

Polymer stringToPolymer(String polymerName);

String getTime(unsigned long millis);

void doBeep();

void doAlarm();

float ruleOfThree(float A, float B, float X);

void IRAM_ATTR watchDogFeed();