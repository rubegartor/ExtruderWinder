#include <Commons/Commons.h>
#include <PID/PIDController.h>
#include <PID_v1.h>

double setPoint, input, output;
double Kp = 5, Ki = 25, Kd = 0.05;

PID autoPID(&input, &output, &setPoint, Kp, Ki, Kd, DIRECT);

void PIDController::init() {
  setPoint = filamentDiameter;
  this->minOutput = 900;
  this->maxOutput = 1800;

  autoPID.SetMode(AUTOMATIC);
}

uint16_t PIDController::computeSpeed() {
  input = calibration.lastRead;
  autoPID.Compute();
  
  return map(output, 0, 255, this->maxOutput, this->minOutput);
}
