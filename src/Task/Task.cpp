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
  BlockNot checkMeasuringPingTimeout(150);
  BlockNot updateTaskedEvents(500);
  BlockNot updateFastTaskedEvents(100);
  BlockNot updateTemperatureSensor(5000);
  BlockNot requestFromScreen(50);

  for (;;) {
    if (updateTemperatureSensor.TRIGGERED) {
      //cooler.refresh();
    }

    if (updateTaskedEvents.TRIGGERED) {
      communication.sendTaskedEvents();
    }

    if (updateFastTaskedEvents.TRIGGERED) {
      communication.sendFastTaskedEvents();
    }

    if (requestFromScreen.TRIGGERED) {
      communication.requestData();
    }

#ifdef ALIGNER_SPI
    if (aligner.isHomed()) {
      aligner.isEnabled(true);

#ifdef DEBUG
      Serial.println("[" + (String)millis() + "] Restarting aligner driver...");
#endif
    }
#endif

#ifdef SPOOLER_SPI
    if (aligner.isHomed()) {
      spooler.isEnabled(true);
#ifdef DEBUG
      Serial.println("[" + (String)millis() + "] Restarting spooler driver...");
#endif
    }
#endif

    spooler.speed = pidSpooler.computeSpeed();

    if (aligner.isHomed()) {
      measuring.read();

      puller.speed = pidPuller.computeSpeed();
    }

    if (isReady() && readTensioner.TRIGGERED) {
      tensioner.run();
    }

    if (checkMeasuringPingTimeout.TRIGGERED) {
      measuring.checkPingTimeout();
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