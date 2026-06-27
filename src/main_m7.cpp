#include "Commons/globals.h"
#include "Commons/pins.h"
#include <rtos.h>

#include "UI/components/general.h"
#include "UI/components/sidebar.h"
#include "UI/components/homeTab.h"
#include "UI/components/controlTab.h"
#include "UI/components/settingsTab.h"
#include "UI/components/popupTab.h"

static rtos::Thread* motorThread = nullptr;

static void motorTask() {
  while (true) {
    aligner.loop();
    puller.loop();
    spooler.loop();
    tensioner.loop();
  }
}

void setup() {
  initGlobals();

  aligner.setInterval(80);
  puller.setInterval(80);
  spooler.setInterval(100);
  tensioner.setInterval(100);
  measurement.setInterval(15);

  puller.setup();
  aligner.setup();
  spooler.setup();
  tensioner.setup();
  measurement.setup();

  build_sidebar();
  build_homeTab(tab1);
  build_controlTab(tab2);
  build_settingsTab(tab3);
  build_popupTab(tab4);

  motorThread = new rtos::Thread(osPriorityNormal, 8192);
  motorThread->start(motorTask);

  setupUITimer();
}

void loop() {
  lv_timer_handler_run_in_period(5);
  measurement.loop();
}

