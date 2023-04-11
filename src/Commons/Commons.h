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

#define DEFAULT_PULLER_SPEED 1500
#define NAMESPACE "extruder"
#define FILAMENT_DIAMETER_MODE_PREF "filaDiam"
#define DEFAULT_FILAMENT_DIAMETER 1.75f

const float spoolMotorRatio = 5.16;
const uint16_t stepsPerCm = 495;  // Steps to travel 1cm
const uint16_t oneRevSpool = 3200 * spoolMotorRatio;
const uint16_t oneRevPuller = 3200;

extern Preferences pref;
extern WifiOut wifiOut;
extern PIDPuller pidPuller;
extern PIDSpooler pidSpooler;
extern Measuring measuring;
extern LCDMenu lcdMenu;
extern Tensioner tensioner;
extern REncoder rotaryEncoder;

// Global variables
extern bool homed;
extern bool needHome;
extern uint16_t spoolTotalRevs;
extern uint16_t pullerTotalRevs;
extern uint16_t spoolSpeed;
extern uint16_t pullerSpeed;
extern float filamentDiameter;
extern long millisOffset;

void commonsInit();

bool isReady();

String getTime(unsigned long millis);

void IRAM_ATTR watchDogFeed();