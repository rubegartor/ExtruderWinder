#include <AccelStepper.h>
#include <Commons/Commons.h>
#include <Winder/Winder.h>

AccelStepper spoolMotor(AccelStepper::DRIVER, SPOOL_STEP_PIN, SPOOL_DIR_PIN);
AccelStepper pullerMotor(AccelStepper::DRIVER, PULLER_STEP_PIN, PULLER_DIR_PIN);

void initWinder() { spoolMotor.setMaxSpeed(SPOOL_MAX_SPEED); }

void IRAM_ATTR winderLoop() {
  spoolSpeed = pidSpooler.computeSpeed();

  if (isReady()) {
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

float getExtrudedLength() {
  float circ = PI * ((float)PULLER_DIAM / (float)1000);
  float value = circ * (float)pullerTotalRevs;

  return circ * (float)pullerTotalRevs;
}

float getExtrudedWeight() {
  Polymer actualPolymer = stringToPolymer(pref.getString(SELECTED_POLYMER_PREF, polymers[0].name));

  return getExtrudedLength() * ruleOfThree(DEFAULT_FILAMENT_DIAMETER, actualPolymer.weight, filamentDiameter); 
}