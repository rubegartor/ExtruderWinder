#include <Arduino.h>
#include "Commons/Commons.h"
#include "Screen/components/general.h"
#include "Screen/components/sidebar.h"
#include "Screen/components/homeTab.h"
#include "Screen/components/controlTab.h"
#include "Screen/components/settingsTab.h"
#include "Screen/components/popupTab.h"

void setup()
{
  Serial.begin(115200);

  SPI1.begin();

  initCommons();

  measurement.setup();
  aligner.setup();
  puller.setup();
  spooler.setup();
  tensioner.setup();
  rf.setup();

  Display.begin();
  TouchDetector.begin();

  build_sidebar();
  build_homeTab(tab1);
  build_controlTab(tab2);
  build_settingsTab(tab3);
  build_popupTab(tab4);

  setupUITimer();
}

void loop() {
  lv_timer_handler_run_in_period(5);

  measurement.loop();
  aligner.loop();
  puller.loop();
  spooler.loop();
  tensioner.loop(250);
  rf.loop(250);

  motorWatchdog();
}
