#pragma once

#include <Arduino.h>
#include <LCD/LCDMenu.h>
#include <Measuring/Measuring.h>
#include <PID/PIDPuller.h>
#include <PID/PIDSPooler.h>
#include <RotaryEncoder/RotaryEncoder.h>
#include <Steppers/Aligner/Aligner.h>
#include <Steppers/Puller/Puller.h>
#include <WiFi/WifiOut.h>
#include <Steppers/Spooler/Spooler.h>
#include <Task/Task.h>
#include <Tensioner/Tensioner.h>
#include <Preferences.h>

#define STALLGUARD_SENSITIVITY 10

#define SPI_MOSI 11
#define SPI_MISO 13
#define SPI_SCK 12

#define BUZZER_PIN 19
#define DEFAULT_SPI_DIR_PIN 1

#define STEPPER_DEF_STEPS 200

#define NAMESPACE "extruder"

#define FILAMENT_DIAMETER_MODE_PREF "filaDiam"
#define DEFAULT_FILAMENT_DIAMETER 1.75f

#define SELECTED_POLYMER_PREF "selPolymer"
#define POLYMER_NUMBER 8


extern Preferences pref;
extern Task task;
extern Aligner aligner;
extern Spooler spooler;
extern Puller puller;
extern WifiOut wifiOut;
extern Measuring measuring;
extern LCDMenu lcdMenu;
extern REncoder rotaryEncoder;
extern PIDPuller pidPuller;
extern PIDSpooler pidSpooler;
extern Tensioner tensioner;

// Global variables
struct Polymer {
  String name;
  float weight;          // Gramos por metro
};

extern bool disableSound;
extern float filamentDiameter;
extern ulong millisOffset;
extern Polymer polymers[POLYMER_NUMBER];

extern long alignerDriverErrorCount;
extern long spoolDriverErrorCount;

void commonsInit();

bool isReady();

Polymer stringToPolymer(String polymerName);

String getTime(unsigned long millis);

void doBeep();

void doAlarm();

float ruleOfThree(float A, float B, float X);

float getExtrudedLength();

float getExtrudedWeight();