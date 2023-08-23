#include <Arduino.h>
#include <Commons/Commons.h>
#include <SPI.h>

void setup() {
#ifdef DEBUG
#warning "DEBUG IS ENABLED!"
  Serial.begin(115200);
  Serial.setDebugOutput(true);
#endif

  lcdMenu.init();

  lcdMenu.println(F("    Conectando..."), 1, true);

  wifiOut.connect();

  lcdMenu.println(F("    Iniciando..."), 1, true);

  pref.begin(NAMESPACE, false);

  SPI.begin();

  commonsInit();

  aligner.init();

  spooler.init();

  puller.init();

  measuring.init();

  rotaryEncoder.init();

  pidPuller.init();

  pidSpooler.init();

  tensioner.init();

  task.initCoreZero();

  task.initCoreOne();
}

void loop() {}