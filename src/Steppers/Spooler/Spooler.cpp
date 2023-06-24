#include <AccelStepper.h>
#include <Commons/Commons.h>
#include <Steppers/Spooler/Spooler.h>
#include <TMCStepper.h>

TMC2130Stepper driverSpool = TMC2130Stepper(SPOOL_CS_PIN, 0.11f);

AccelStepper spoolMotor(AccelStepper::DRIVER, SPOOL_STEP_PIN, 0);

void Spooler::configDriver() {
  driverSpool.begin();              // Initiate pins and registers
  driverSpool.toff(4);              // off time
  driverSpool.blank_time(24);       // blank time
  driverSpool.rms_current(850);     // 850mAh RMS
  driverSpool.microsteps(4);        // 4 microsteps
  driverSpool.en_pwm_mode(true);    // Enable StealthChop
  driverSpool.pwm_autoscale(true);  // StealthChop
}

void Spooler::init() {
  pinMode(SPOOL_STEP_PIN, OUTPUT);

  spoolMotor.setMaxSpeed(SPOOL_MAX_SPEED);
}

void Spooler::run() {
  if (measuring.autoStopStatus == autoStopTriggered) return;

  if (isReady()) {
    spoolMotor.setSpeed(this->speed);
    spoolMotor.runSpeed();

    if (spoolMotor.currentPosition() >= oneRevSpool) {
      this->totalRevs++;
      spoolMotor.setCurrentPosition(0);
    }
  }
}