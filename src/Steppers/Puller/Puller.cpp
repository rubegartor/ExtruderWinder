#include <Commons/Commons.h>
#include <Steppers/Puller/Puller.h>

AccelStepper pullerMotor(AccelStepper::DRIVER, PULLER_STEP_PIN, 0);

void Puller::init() { pullerMotor.setMaxSpeed(PULLER_MAX_SPEED); }

void Puller::run() {
  if (measuring.autoStopStatus == autoStopTriggered) return;

  pullerMotor.setSpeed(this->speed);
  pullerMotor.runSpeed();

  if (pullerMotor.currentPosition() >= oneRevPuller) {
    this->totalRevs++;
    pullerMotor.setCurrentPosition(0);
  }
}