#include <AccelStepper.h>
#include <Commons/Commons.h>
#include <Winder/Winder.h>

AccelStepper spoolMotor(AccelStepper::DRIVER, SPOOL_STEP_PIN, SPOOL_DIR_PIN);
AccelStepper pullerMotor(AccelStepper::DRIVER, PULLER_STEP_PIN, PULLER_DIR_PIN);

void initWinder() { spoolMotor.setMaxSpeed(SPOOL_MAX_SPEED); }

void IRAM_ATTR winderLoop() {
  if (homed) {
    spoolSpeed = pidSpooler.computeSpeed();

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
    pullerLoop();
    winderLoop();

    watchDogFeed();
  }
}

float getExtrudedLength() {
  float circ = PI * ((float)PULLER_DIAM / (float)1000);

  return circ * (float)pullerTotalRevs;
}