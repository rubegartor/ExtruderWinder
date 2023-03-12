#include <AiEsp32RotaryEncoder.h>
#include <Arduino.h>
#include <Commons/Commons.h>
#include <LCD/LCDMenu.h>
#include <RotaryEncoder/RotaryEncoder.h>
#include <Tensioner/Tensioner.h>
#include <Calibration/Calibration.h>

#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"

LCDMenu lcdMenu = LCDMenu();
Tensioner tensioner = Tensioner();
REncoder rotaryEncoder = REncoder();
Calibration calibration = Calibration();

bool homed;
bool needHome;
bool firstSync;
bool pullerState = true;
uint16_t actualDistance;
uint16_t spoolTotalRevs;
uint16_t pullerTotalRevs;
uint16_t spoolSpeed = pullerSpeed * speedRatioMultiplier;
uint16_t pullerSpeed = 1500;

bool isReady() { return homed && !needHome; }

void IRAM_ATTR watchDogFeed() {
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed = 1;
  TIMERG0.wdt_wprotect = 0;
}