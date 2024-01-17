#include <Arduino.h>
#include <Commons/Commons.h>
#include <SPI.h>

void setup() {
#ifdef DEBUG
#warning "DEBUG IS ENABLED!"
  Serial.begin(115200);
  Serial.setDebugOutput(true);
#endif

  pref.begin(NAMESPACE, false);

  communication.init();

  SPI.begin();

  commonsInit();

  aligner.init();

  spooler.init();

  puller.init();

  measuring.init();

  pidPuller.init();

  pidSpooler.init();

  tensioner.init();

  cooler.init();

  task.initCoreZero();

  task.initCoreOne();
}

void loop() {}