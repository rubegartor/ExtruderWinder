#include <Arduino.h>
#include <Commons/Commons.h>
#include <SPI.h>

void setup() {
  Serial.begin(115200);  // Solo para debug
  Serial2.begin(115200);
  while (!Serial2)
    ;

  Serial.println(F("Starting Serial2..."));

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  SPI.begin();

  pinMode(MISO, INPUT_PULLUP);

  aligner.configDriver();

  spooler.configDriver();

  pref.begin(NAMESPACE, false);

  commonsInit();

  measuring.init();

  lcdMenu.init();

  lcdMenu.println(F("    Iniciando..."), 1, true);

  wifiOut.connect();

  tensioner.init();

  rotaryEncoder.init();

  pidPuller.init();

  pidSpooler.init();

  task.initCoreZero();

  task.initCoreOne();
}

void loop() {}