#pragma once

#include <Arduino.h>

#include <Connection/Connection.h>
#include <LCD/LCDMenu.h>
#include <Tensioner/Tensioner.h>
#include <RotaryEncoder/RotaryEncoder.h>

#define ROTARY_ENCODER_A_PIN 27       // DT
#define ROTARY_ENCODER_B_PIN 26       // CLK
#define ROTARY_ENCODER_BUTTON_PIN 35  // SW
#define ROTARY_ENCODER_STEPS 4
#define ROTARY_ENCODER_VCC_PIN -1
#define RORATY_ENCODER_ACCELERATION 120

const float spoolMotorRatio = 5.18;
const uint16_t stepsPerCm = 476;                 // Steps to travel 1cm
const uint16_t oneRev = 3200 * spoolMotorRatio;  // Steps for 1 revolution at  microsteps
const float filamentDiameter = 0.175;            // 1.75mm

const uint16_t minDistance = 55;
const uint16_t offsetDistance = 100;

extern ESPNowConnection espnow;
extern LCDMenu lcdMenu;
extern Tensioner tensioner;
extern REncoder rotaryEncoder;

// Global variables
extern bool homed;
extern bool canLCD;
extern bool spool;
extern int selectedOption;
extern uint16_t totalRevs;
extern uint16_t actualDistance;
extern uint16_t spoolSpeed;


IRAM_ATTR void watchDogFeed();