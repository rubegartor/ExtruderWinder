#include <Commons/Commons.h>
#include <Steppers/Puller/Puller.h>

TMC2130Stepper pullerDriver = TMC2130Stepper(PULLER_CS_PIN, 0.11f, SPI_MOSI, SPI_MISO, SPI_SCK);

AccelStepper pullerMotor(AccelStepper::DRIVER, PULLER_STEP_PIN, DEFAULT_SPI_DIR_PIN);

void Puller::init() {
  pinMode(PULLER_STEP_PIN, OUTPUT);

#ifdef PULLER_SPI
  this->configDriver();
#endif

  pullerMotor.setMaxSpeed(PULLER_MAX_SPEED);
}

void Puller::configDriver() {
  pullerDriver.begin();                        // Initiate pins and registers
  pullerDriver.toff(4);                        // off time
  pullerDriver.blank_time(24);                 // blank time
  pullerDriver.rms_current(550);               // 550mAh RMS
  pullerDriver.microsteps(PULLER_MICROSTEPS);  // 16 microsteps
  pullerDriver.en_pwm_mode(true);              // Enable StealthChop
  pullerDriver.pwm_autoscale(true);            // StealthChop
  pullerDriver.shaft(true);
}

void Puller::run() {
  if (measuring.autoStopStatus == autoStopTriggered) return;

  pullerMotor.setSpeed(this->speed);
  pullerMotor.runSpeed();

  if (pullerMotor.currentPosition() >= PULLER_ONE_REV_STEPS) {
    this->totalRevs++;
    pullerMotor.setCurrentPosition(0);
  }
}