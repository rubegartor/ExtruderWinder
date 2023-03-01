#include <Arduino.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include <Commons/Commons.h>
#include <AiEsp32RotaryEncoder.h>
#include <Connection/Connection.h>
#include <LCD/LCDMenu.h>
#include <Tensioner/Tensioner.h>
#include <RotaryEncoder/RotaryEncoder.h>

ESPNowConnection espnow = ESPNowConnection();
LCDMenu lcdMenu = LCDMenu();
Tensioner tensioner = Tensioner();
REncoder rotaryEncoder = REncoder();

bool homed;
bool canLCD;
bool spool = true;
int selectedOption;
uint16_t totalRevs;
uint16_t actualDistance;
uint16_t spoolSpeed = 2000;

IRAM_ATTR void watchDogFeed()
{
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
}