#pragma once

#include <AiEsp32RotaryEncoder.h>
#include <Arduino.h>
#include <Commons/Commons.h>
#include <LCD/LCDMenu.h>
#include <RotaryEncoder/RotaryEncoder.h>
#include <Tensioner/Tensioner.h>
#include <Measuring/Measuring.h>
#include <PID/PIDPuller.h>
#include <PID/PIDSpooler.h>
#include <Preferences.h>

#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"

#define DEFAULT_PULLER_SPEED 1500
#define NAMESPACE "extruder"

const float spoolMotorRatio = 5.18;
const uint16_t stepsPerCm = 476;  // Steps to travel 1cm
const float speedRatioMultiplier = 1.3; // Speed ratio between spool speed and puller speed
const uint16_t oneRevSpool = 3200 * spoolMotorRatio;
const uint16_t oneRevPuller = 3200;
const float filamentDiameter = 1.75;

extern Preferences pref;
extern PIDPuller pidPuller;
extern PIDSpooler pidSpooler;
extern Measuring measuring;
extern LCDMenu lcdMenu;
extern Tensioner tensioner;
extern REncoder rotaryEncoder;

// Global variables
extern bool homed;
extern bool needHome;
extern bool pullerState;
extern uint16_t actualDistance;
extern uint16_t spoolTotalRevs;
extern uint16_t pullerTotalRevs;
extern uint16_t spoolSpeed;
extern uint16_t pullerSpeed;

bool isReady();

void IRAM_ATTR watchDogFeed();