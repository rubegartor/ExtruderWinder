#include <Aligner/Aligner.h>
#include <Arduino.h>
#include <Commons/Commons.h>
#include <TMCStepper.h>
#include <Winder/Winder.h>

#define R_SENSE 0.11f  // TMC2130

TMC2130Stepper driverAligner = TMC2130Stepper(ALIGNER_CS_PIN, R_SENSE);
TMC2130Stepper driverSpool = TMC2130Stepper(SPOOL_CS_PIN, R_SENSE);

using namespace TMC2130_n;

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

  Serial2.println(F("Starting Serial2..."));

  pinMode(SPOOL_CS_PIN, OUTPUT);
  digitalWrite(SPOOL_CS_PIN, HIGH);

  pinMode(ALIGNER_CS_PIN, OUTPUT);
  digitalWrite(ALIGNER_CS_PIN, HIGH);

  SPI.begin();

  pinMode(MISO, INPUT_PULLUP);

  driverAligner.begin();           // Initiate pins and registers
  driverAligner.toff(4);           // off time
  driverAligner.blank_time(24);    // blank time
  driverAligner.rms_current(400);  // 600mAh RMS
  driverAligner.microsteps(2);     // 2 microsteps

  driverSpool.begin();              // Initiate pins and registers
  driverSpool.toff(4);              // off time
  driverSpool.blank_time(24);       // blank time
  driverSpool.rms_current(600);     // 600mAh RMS
  driverSpool.microsteps(16);       // 16 microsteps
  driverSpool.en_pwm_mode(true);    // Enable StealthChop
  driverSpool.pwm_autoscale(true);  // StealthChop

  pinMode(SPOOL_DIR_PIN, OUTPUT);
  pinMode(SPOOL_STEP_PIN, OUTPUT);

  digitalWrite(SPOOL_DIR_PIN, LOW);

  pinMode(ALIGNER_DIR_PIN, OUTPUT);
  pinMode(ALIGNER_STEP_PIN, OUTPUT);

  digitalWrite(ALIGNER_DIR_PIN, LOW);

  lcdMenu.init();  // Iniciar el LCD

  lcdMenu.println(F("    Iniciando..."), 1, true);

  delay(1500);

  tensioner.init();  // Iniciar el sensor de distancia del tensionador

  rotaryEncoder.init();  // Iniciar el rotary encoder

  pinMode(ALIGNER_HOME_SENSOR_PIN, INPUT_PULLUP);

  // espnow.init();  // Iniciar la conexión inalámbrica a través de ESP-Now

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