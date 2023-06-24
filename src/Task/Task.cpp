#include <BlockNot.h>
#include <Commons/Commons.h>
#include <Task/Task.h>

#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"

TaskHandle_t coreZeroTask;
TaskHandle_t coreOneTask;

void IRAM_ATTR _watchDogFeed() {
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed = 1;
  TIMERG0.wdt_wprotect = 0;
}

void _coreOneImpl(void* pvParameters) {
  puller.init();
  spooler.init();
  aligner.init();

  for (;;) {
    puller.run();
    spooler.run();
    aligner.run();

    if (measuring.autoStopStatus == autoStopTriggered) {
      doAlarm();
    }

    _watchDogFeed();
  }
}

void _coreZeroImpl(void* pvParameters) {
  BlockNot readTensioner(200);
  BlockNot updateSummary(500);
  BlockNot updateWifiEvents(500);

  for (;;) {
    delay(5);

    spooler.speed = pidSpooler.computeSpeed();

    if (updateWifiEvents.TRIGGERED && wifiOut.connected) wifiOut.handleEvents();

    if (aligner.isHomed()) {
      measuring.read();
    }

    if (aligner.isHomed() && measuring.mode == measuringAutoMode) {
      puller.speed = pidPuller.computeSpeed();
    }

    if (aligner.isHomed() && updateSummary.TRIGGERED) {
      if (lcdMenu.inSummary) {
        lcdMenu.initSummary();
      }
    }

    if (aligner.isHomed() && aligner.alignerActualStatus != alignerStart) {
      if (rotaryEncoder.changed()) {
        lcdMenu.onREncoderChange(rotaryEncoder);
      }

      if (rotaryEncoder.clicked()) {
        lcdMenu.onREncoderClick(rotaryEncoder);
      }

      if (lcdMenu.menuPosition == pullerSpeedOption) {
        lcdMenu.checkLCDButtons();
      }
    }

    if (isReady()) {
      if (readTensioner.TRIGGERED) tensioner.getDistance();
    }

    _watchDogFeed();
  }
}

void Task::initCoreZero() {
  xTaskCreatePinnedToCore(
      _coreZeroImpl,  /* Task function. */
      "coreZeroTask", /* name of task. */
      50000,          /* Stack size of task */
      NULL,           /* parameter of the task */
      10,             /* priority of the task */
      &coreZeroTask,  /* Task handle to keep track of created task */
      0);             /* pin task to core 0 */
}

void Task::initCoreOne() {
  xTaskCreatePinnedToCore(
      _coreOneImpl,  /* Task function. */
      "coreOneTask", /* name of task. */
      10000,         /* Stack size of task */
      NULL,          /* parameter of the task */
      10,            /* priority of the task */
      &coreOneTask,  /* Task handle to keep track of created task */
      1);            /* pin task to core 1 */
}