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

#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"

Preferences pref;
PIDPuller pidPuller;
PIDSpooler pidSpooler;
LCDMenu lcdMenu;
Tensioner tensioner;
REncoder rotaryEncoder;
Measuring measuring;

bool homed;
bool needHome;
uint16_t actualDistance;
uint16_t spoolTotalRevs;
uint16_t pullerTotalRevs;
uint16_t spoolSpeed = DEFAULT_WINDER_SPEED;
uint16_t pullerSpeed = DEFAULT_PULLER_SPEED;
float filamentDiameter = DEFAULT_FILAMENT_DIAMETER;

void commonsInit() {
  filamentDiameter =
      pref.getFloat(FILAMENT_DIAMETER_MODE_PREF, DEFAULT_FILAMENT_DIAMETER);

  pidPuller.updateSetPoint(filamentDiameter);
}

bool isReady() { return homed && !needHome; }

void IRAM_ATTR watchDogFeed() {
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed = 1;
  TIMERG0.wdt_wprotect = 0;
}