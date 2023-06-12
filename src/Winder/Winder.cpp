#include <AccelStepper.h>
#include <Commons/Commons.h>
#include <TMCStepper.h>
#include <Winder/Winder.h>

TMC2130Stepper driverSpool = TMC2130Stepper(SPOOL_CS_PIN, R_SENSE);

AccelStepper spoolMotor(AccelStepper::DRIVER, SPOOL_STEP_PIN, 0);
AccelStepper pullerMotor(AccelStepper::DRIVER, PULLER_STEP_PIN, 0);

bool alignerTestRun;

void configSpoolDriver() {
  driverSpool.begin();              // Initiate pins and registers
  driverSpool.toff(4);              // off time
  driverSpool.blank_time(24);       // blank time
  driverSpool.rms_current(850);     // 850mAh RMS
  driverSpool.microsteps(4);        // 4 microsteps
  driverSpool.en_pwm_mode(true);    // Enable StealthChop
  driverSpool.pwm_autoscale(true);  // StealthChop
}

void initWinder() { spoolMotor.setMaxSpeed(SPOOL_MAX_SPEED); }

void IRAM_ATTR winderLoop() {
  spoolSpeed = pidSpooler.computeSpeed();

  if (isReady() || alignerTestRun) {
    if (alignerTestRun) spoolSpeed = SPOOL_MAX_SPEED / 2;

    spoolMotor.setSpeed(spoolSpeed);
    spoolMotor.runSpeed();

    if (spoolMotor.currentPosition() >= oneRevSpool) {
      spoolTotalRevs++;
      spoolMotor.setCurrentPosition(0);
    }
  }
}

void initPuller() { pullerMotor.setMaxSpeed(PULLER_MAX_SPEED); }

void IRAM_ATTR pullerLoop() {
  pullerMotor.setSpeed(pullerSpeed);
  pullerMotor.runSpeed();

  if (pullerMotor.currentPosition() >= oneRevPuller) {
    pullerTotalRevs++;
    pullerMotor.setCurrentPosition(0);
  }
}

void IRAM_ATTR wTask(void* pvParameters) {
  initWinder();
  initPuller();

  for (;;) {
    if (pidPuller.inAutoStop()) doAlarm();

    pullerLoop();
    winderLoop();

    watchDogFeed();
  }
}

void toggleAlignerTestRun() { alignerTestRun = !alignerTestRun; }

float getExtrudedLength() {
  float circ = PI * ((float)PULLER_DIAM / 1000.0f);
  float value = circ * (float)pullerTotalRevs;

  return circ * (float)pullerTotalRevs;
}

float getExtrudedWeight() {
  Polymer actualPolymer =
      stringToPolymer(pref.getString(SELECTED_POLYMER_PREF, polymers[0].name));

  return getExtrudedLength() * ruleOfThree(DEFAULT_FILAMENT_DIAMETER,
                                           actualPolymer.weight,
                                           filamentDiameter);
}