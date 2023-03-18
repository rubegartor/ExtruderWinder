#include <Commons/Commons.h>
#include <PID/PIDController.h>
#include <PID_v1.h>

double setPoint, input, output;
double Kp = 5, Ki = 25, Kd = 0.05;

PID autoPID(&input, &output, &setPoint, Kp, Ki, Kd, DIRECT);

void PIDController::init() {
  setPoint = filamentDiameter;
  this->minOutput = 1000;
  this->maxOutput = 1800;

  autoPID.SetMode(AUTOMATIC);
}

bool PIDController::isValidToCompute(float input) {
  return abs(this->lastComputedDiameter - input) <= MAX_VALID_DEVIATION;
}

uint16_t PIDController::computeSpeed() {
  input = measuring.lastRead;

  if (this->stabilized && this->isValidToCompute(input)) {
    autoPID.Compute();

    this->lastComputedDiameter = input;
    this->lastComputed = map(output, 0, 255, this->maxOutput, this->minOutput);
  }

  return this->lastComputed;
}
