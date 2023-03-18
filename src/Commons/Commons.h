#pragma once

#include <AiEsp32RotaryEncoder.h>
#include <Arduino.h>
#include <Commons/Commons.h>
#include <LCD/LCDMenu.h>
#include <RotaryEncoder/RotaryEncoder.h>
#include <Tensioner/Tensioner.h>
#include <Measuring/Measuring.h>
#include <PID/PIDController.h>

#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"

const float spoolMotorRatio = 5.18;
const uint16_t stepsPerCm = 476;  // Steps to travel 1cm
const float speedRatioMultiplier = 1.25; // Speed ratio between spool speed and puller speed
const uint16_t oneRevSpool = 3200 * spoolMotorRatio;
const uint16_t oneRevPuller = 3200;
const float filamentDiameter = 1.75;

extern PIDController pid;
extern Measuring measuring;
extern LCDMenu lcdMenu;
extern Tensioner tensioner;
extern REncoder rotaryEncoder;

// Global variables
extern bool homed;
extern bool needHome;
extern bool firstSync;
extern bool pullerState;
extern bool automaticPuller;
extern uint16_t actualDistance;
extern uint16_t spoolTotalRevs;
extern uint16_t pullerTotalRevs;
extern uint16_t spoolSpeed;
extern uint16_t pullerSpeed;

bool isReady();

void IRAM_ATTR watchDogFeed();