#include <Winder/Winder.h>
#include <AccelStepper.h>
#include <Commons/Commons.h>

AccelStepper spoolMotor(AccelStepper::DRIVER, SPOOL_STEP_PIN, SPOOL_DIR_PIN);

void winder(void* pvParameters) {
  spoolMotor.setMaxSpeed(SPOOL_MAX_SPEED);

  bool run = true;
  bool minTriggered = false;

  for (;;) {
    if (actualDistance <= minDistance) {
      minTriggered = true;
      run = false;
    }

    if (minTriggered && (actualDistance - offsetDistance) > 60) {
      run = true;
      minTriggered = false;
    }

    if (homed && run && spool) {
      spoolMotor.setSpeed(spoolSpeed);
      spoolMotor.runSpeed();

      if (spoolMotor.currentPosition() >= oneRev) {
        totalRevs += 1;
        spoolMotor.setCurrentPosition(0);
      }
    }

    watchDogFeed();
  }
}