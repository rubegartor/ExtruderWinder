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

  Display.begin();
  TouchDetector.begin();

  build_sidebar();
  build_homeTab(tab1);
  build_controlTab(tab2);
  build_settingsTab(tab3);
  build_popupTab(tab4);

  setupUITimer();
}

unsigned long lvglMillis = 0;

void loop() {
  if (millis() - lvglMillis >= 5) {
    lv_task_handler();
    lvglMillis = millis();
  }

  aligner.loop();
  puller.loop();
  spooler.loop();
  tensioner.loop(250);
  measurement.loop();
}