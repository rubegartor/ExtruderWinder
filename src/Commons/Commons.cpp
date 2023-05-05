#include <AiEsp32RotaryEncoder.h>
#include <Aligner/Aligner.h>
#include <Arduino.h>
#include <Commons/Commons.h>
#include <LCD/LCDMenu.h>
#include <Measuring/Measuring.h>
#include <PID/PIDPuller.h>
#include <PID/PIDSpooler.h>
#include <Preferences.h>
#include <RotaryEncoder/RotaryEncoder.h>
#include <Tensioner/Tensioner.h>

#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"

Preferences pref;
WifiOut wifiOut;
PIDPuller pidPuller;
PIDSpooler pidSpooler;
LCDMenu lcdMenu;
Tensioner tensioner;
REncoder rotaryEncoder;
Measuring measuring;

Polymer ABS;
Polymer PLA;
Polymer TPU80;
Polymer TPU85;
Polymer TPU90;
Polymer PETG;
Polymer PCABS;
Polymer OTHER;

bool homed;
bool needHome;
uint16_t spoolTotalRevs;
uint16_t pullerTotalRevs;
uint16_t spoolSpeed = 0;
uint16_t pullerSpeed = DEFAULT_PULLER_SPEED;
float filamentDiameter = DEFAULT_FILAMENT_DIAMETER;
ulong millisOffset = 0;
Polymer polymers[POLYMER_NUMBER];

Polymer stringToPolymer(String polymerName) {
  if (polymerName == "PLA") return polymers[0];
  if (polymerName == "ABS") return polymers[1];
  if (polymerName == "TPU-80") return polymers[2];
  if (polymerName == "TPU-85") return polymers[3];
  if (polymerName == "TPU-90") return polymers[4];
  if (polymerName == "PETG") return polymers[5];
  if (polymerName == "PC/ABS") return polymers[6];
  if (polymerName == "Otro") return polymers[7];

  return polymers[0];
}

void initPolymers() {
  PLA.name = "PLA";
  PLA.weight = 2.98f;
  PLA.diameterOffset = 0.00f;
  polymers[0] = PLA;

  ABS.name = "ABS";
  ABS.weight = 2.45f;
  ABS.diameterOffset = 0.00f;
  polymers[1] = ABS;

  TPU80.name = "TPU-80";
  TPU80.weight = 3.26f;
  TPU80.diameterOffset = 0.17f;
  polymers[2] = TPU80;

  TPU85.name = "TPU-85";
  TPU85.weight = 3.26f;
  TPU85.diameterOffset = 0.13f;
  polymers[3] = TPU85;

  TPU90.name = "TPU-90";
  TPU90.weight = 3.52f;
  TPU90.diameterOffset = 0.08f;
  polymers[4] = TPU90;

  PETG.name = "PETG";
  PETG.weight = 3.05f;
  PETG.diameterOffset = 0.00f;
  polymers[5] = PETG;

  PCABS.name = "PC/ABS";
  PCABS.weight = 2.60f;
  PCABS.diameterOffset = 0.00f;
  polymers[6] = PCABS;

  OTHER.name = "Otro";
  OTHER.weight = 0.00f;
  OTHER.diameterOffset = 0.00f;
  polymers[7] = OTHER;
}

void commonsInit() {
  initPolymers();

  filamentDiameter =
      pref.getFloat(FILAMENT_DIAMETER_MODE_PREF, DEFAULT_FILAMENT_DIAMETER);

  pidPuller.updateSetPoint(filamentDiameter);
}

bool isHomed() { return homed && !needHome; }

bool isReady() { return isHomed() && isPositioned(); }

String getTime(unsigned long millis) {
  ulong seconds = (millis / 1000) % 60;
  ulong minutes = (millis / (1000 * 60)) % 60;
  ulong hours = (millis / (1000 * 60 * 60)) % 24;

  char time_str[9];
  sprintf(time_str, "%02d:%02d:%02d", hours, minutes, seconds);
  return String(time_str);
}

void doBeep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(5);
  digitalWrite(BUZZER_PIN, LOW);
}

void IRAM_ATTR watchDogFeed() {
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed = 1;
  TIMERG0.wdt_wprotect = 0;
}