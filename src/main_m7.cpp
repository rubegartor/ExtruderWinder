#include "Commons/globals.h"
#include "Commons/pins.h"
#include "Commons/EventLog.h"
#include <rtos.h>

#include "UI/components/general.h"
#include "UI/ui.h"

static rtos::Thread* motorThread = nullptr;

static void motorTask() {
  while (true) {
    aligner.loop();
    puller.loop();
    spooler.loop();
    tensioner.loop();
    g_beatMotor = millis();
    // Ceder la CPU: sin esto el hilo nunca se bloquea y la UI/medición solo corren
    // cuando el round-robin del RTOS le quita el procesador (cada 5 ms).
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(1));
  }
}

void setup() {
  initGlobals();

  puller.setup(80);
  aligner.setup(80);
  spooler.setup(100);
  tensioner.setup(100);
  measurement.setup(15);

  build_ui();

  motorThread = new rtos::Thread(osPriorityNormal, 8192);
  motorThread->start(motorTask);

  eventLogBegin();
}

void loop() {
  lv_timer_handler_run_in_period(5);
  measurement.loop();
  g_beatUi = millis();
}

