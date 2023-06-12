#include <Aligner/Aligner.h>
#include <Arduino.h>
#include <Commons/Commons.h>
#include <SPI.h>
#include <Winder/Winder.h>

TaskHandle_t winderTask;
TaskHandle_t alignerTask;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println(F("Starting..."));

  Serial2.begin(115200);
  while (!Serial2)
    ;

  Serial.println(F("Starting Serial2..."));

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(SPOOL_CS_PIN, OUTPUT);
  digitalWrite(SPOOL_CS_PIN, HIGH);

  pinMode(ALIGNER_CS_PIN, OUTPUT);
  digitalWrite(ALIGNER_CS_PIN, HIGH);

  SPI.begin();

  pinMode(MISO, INPUT_PULLUP);

  configAlignerDriver();

  configSpoolDriver();

  pinMode(SPOOL_STEP_PIN, OUTPUT);

  pinMode(ALIGNER_DIR_PIN, OUTPUT);
  pinMode(ALIGNER_STEP_PIN, OUTPUT);

  digitalWrite(ALIGNER_DIR_PIN, LOW);

  pref.begin(NAMESPACE, false);

  commonsInit();

  measuring.init();

  lcdMenu.init();  // Iniciar el LCD

  lcdMenu.println(F("    Iniciando..."), 1, true);

  delay(1500);

  tensioner.init();  // Iniciar el sensor de distancia del tensionador

  rotaryEncoder.init();  // Iniciar el rotary encoder

  pidPuller.init();

  pidSpooler.init();

  xTaskCreatePinnedToCore(
      wTask,       /* Task function. */
      "Winder",    /* name of task. */
      10000,       /* Stack size of task */
      NULL,        /* parameter of the task */
      10,          /* priority of the task */
      &winderTask, /* Task handle to keep track of created task */
      0);          /* pin task to core 0 */

  xTaskCreatePinnedToCore(
      aTask,        /* Task function. */
      "Aligner",    /* name of task. */
      50000,        /* Stack size of task */
      NULL,         /* parameter of the task */
      10,           /* priority of the task */
      &alignerTask, /* Task handle to keep track of created task */
      1);           /* pin task to core 1 */
}

void loop() {}