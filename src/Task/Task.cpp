#include <BlockNot.h>
#include <Commons/Commons.h>
#include <Task/Task.h>

#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"

TaskHandle_t coreZeroTask;
TaskHandle_t coreOneTask;

void IRAM_ATTR _watchDogFeed() {
  TIMERG0.wdtwprotect.wdt_wkey = TIMG_WDT_WKEY_V;
  TIMERG0.wdtfeed.wdt_feed = 1;
  TIMERG0.wdtwprotect.wdt_wkey = 0;
}

void _coreOneImpl(void* pvParameters) {
  puller.init();
  spooler.init();
  aligner.init();

  for (;;) {
    if (measuring.autoStopStatus == autoStopTriggered && !disableSound) {
      doAlarm();
    } else {
      puller.run();
      spooler.run();
      aligner.run();
    }

    _watchDogFeed();
  }
}

void _coreZeroImpl(void* pvParameters) {
  BlockNot readTensioner(150);
  BlockNot updateSummary(500);
  BlockNot updateWifiEvents(1000);

  for (;;) {
    if (aligner.alignerActualStatus != alignerPositioned) {
      delay(5);  // No delay = Core 0 crash
    }

    if (task.initSummary) {
      lcdMenu.initSummary(true);

      task.initSummary = false;
    }

    if (task.printStartPos) {
      lcdMenu.println(" Elige el inicio de", 1, true);
      lcdMenu.println("     la bobina", 2);

      task.printStartPos = false;
    }

    if (task.printEndPos) {
      lcdMenu.println(" Elige el final de", 1, true);
      lcdMenu.println("     la bobina", 2);

      task.printEndPos = false;
    }

    if (updateWifiEvents.TRIGGERED) wifiOut.handleEvents();

#ifdef ALIGNER_SPI
    if (aligner.isHomed() && !aligner.isEnabled(true)) {
      alignerDriverErrorCount++;

#ifdef DEBUG
      Serial.println("[" + (String)millis() + "] Restarting aligner driver...");
#endif
    }
#endif

#ifdef SPOOLER_SPI
    if (aligner.isHomed() && !spooler.isEnabled(true)) {
      spoolDriverErrorCount++;
#ifdef DEBUG
      Serial.println("[" + (String)millis() + "] Restarting spooler driver...");
#endif
    }
#endif

    spooler.speed = pidSpooler.computeSpeed();

    if (aligner.isHomed()) {
      measuring.read();
    }

    if (aligner.isHomed() && measuring.mode == measuringAutoMode) {
      puller.speed = pidPuller.computeSpeed();
    }

    if (aligner.isHomed() && updateSummary.TRIGGERED && lcdMenu.inSummary) {
      lcdMenu.initSummary();
    }

    if (aligner.isHomed() && aligner.alignerActualStatus != alignerStart) {
      // No delay when inside lcd menu = Core 0 crash
      delay(5);

      if (rotaryEncoder.changed()) {
        lcdMenu.onREncoderChange(rotaryEncoder);
      }

      if (rotaryEncoder.clicked()) {
        if (measuring.autoStopStatus == autoStopTriggered) disableSound = true;

        lcdMenu.onREncoderClick(rotaryEncoder);
      }
    }

    if (isReady() && readTensioner.TRIGGERED) {
      tensioner.run();
    }

    _watchDogFeed();
  }
}

void Task::initCoreZero() {
  xTaskCreatePinnedToCore(
      _coreZeroImpl,  /* Task function. */
      "coreZeroTask", /* name of task. */
      10000,          /* Stack size of task */
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