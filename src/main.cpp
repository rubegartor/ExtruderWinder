#include <Arduino.h>
#include <Commons/Commons.h>
#include <SPI.h>

void setup() {
  Serial.begin(115200);

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