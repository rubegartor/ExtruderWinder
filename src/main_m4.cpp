#include "Commons/globals.h"
#include "Commons/pins.h"

#include "UI/components/general.h"
#include "UI/components/sidebar.h"
#include "UI/components/homeTab.h"
#include "UI/components/controlTab.h"
#include "UI/components/settingsTab.h"
#include "UI/components/popupTab.h"

void setup() {  
  initGlobals();

  aligner.setInterval(80);
  puller.setInterval(80);
  spooler.setInterval(100);
  tensioner.setInterval(100);

#ifndef WITHOUT_MOTOR_CONTROL
  puller.setup();
  aligner.setup();
  spooler.setup();
#endif
  tensioner.setup();

  build_sidebar();
  build_homeTab(tab1);
  build_controlTab(tab2);
  build_settingsTab(tab3);
  build_popupTab(tab4);

  setupUITimer();
}

void loop() {
  lv_timer_handler_run_in_period(5);

  rpcManager.loop();
  if (rpcManager.hasNewMeasurementData()) {
    const MeasurementDataMessage& data = rpcManager.getMeasurementData();
    
    measurementLastRead = data.lastRead;
    measurementMinRead = data.minRead;
    measurementMaxRead = data.maxRead;

    addChartValue(static_cast<int32_t>(data.lastRead * 100));

    rpcManager.markMeasurementDataProcessed();
  }

  aligner.loop();
  puller.loop();
  spooler.loop();
  tensioner.loop();
}
