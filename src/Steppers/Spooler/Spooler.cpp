#include <AccelStepper.h>
#include <Commons/Commons.h>
#include <Steppers/Spooler/Spooler.h>
#include <TMCStepper.h>

TMC2130Stepper driverSpool = TMC2130Stepper(SPOOL_CS_PIN, 0.11f, SPI_MOSI, SPI_MISO, SPI_SCK);

AccelStepper spoolMotor(AccelStepper::DRIVER, SPOOL_STEP_PIN, DEFAULT_SPI_DIR_PIN);

void Spooler::configDriver() {
  driverSpool.begin();                       // Initiate pins and registers
  driverSpool.rms_current(850);              // 850mAh RMS
  driverSpool.microsteps(SPOOL_MICROSTEPS);  // 8 microsteps
  driverSpool.en_pwm_mode(true);             // Enable StealthChop
  driverSpool.pwm_autoscale(true);           // StealthChop
  driverSpool.shaft(true);
}

void Spooler::init() {
#ifdef SPOOLER_SPI
  this->configDriver();
#endif

  pinMode(SPOOL_STEP_PIN, OUTPUT);

  spoolMotor.setMaxSpeed(SPOOL_MAX_SPEED);
}

void Spooler::run() {
  if (measuring.autoStopStatus == autoStopTriggered) return;

  if (isReady()) {
    spoolMotor.setSpeed(this->speed);
    spoolMotor.runSpeed();

    if (spoolMotor.currentPosition() >= SPOOL_ONE_REV_STEPS) {
      this->totalRevs++;
      spoolMotor.setCurrentPosition(0);
    }
  }
}

bool Spooler::isEnabled(bool restartIfDisabled) {
  bool enabled = driverSpool.isEnabled();

  if (restartIfDisabled && !enabled) this->configDriver();

  return enabled;
}