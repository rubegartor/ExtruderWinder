#include <Commons/Commons.h>
#include <PID/PIDPuller.h>
#include <PID_v1.h>

double setPoint, input, output;
double aggKp = 5, aggKi = 25, aggKd = 0.05;
double Kp = 2, Ki = 12.5, Kd = 1;

PID autoPID(&input, &output, &setPoint, Kp, Ki, Kd, DIRECT);

void PIDPuller::init() {
  setPoint = filamentDiameter;
  this->minOutput = 1000;
  this->maxOutput = 2000;

  this->stabilized = false;

  autoPID.SetMode(AUTOMATIC);
}

uint16_t PIDPuller::computeSpeed() {
  input = measuring.lastRead;

  double gap = abs(setPoint - input);

  if (!this->stabilized && gap > PID_AGGRESSIVE_GAP) {
    autoPID.SetTunings(aggKp, aggKi, aggKd);
  } else {
    autoPID.SetTunings(Kp, Ki, Kd);
  }

  this->doCompute(input);

  return this->lastComputed;
}

void PIDPuller::doCompute(float input) {
  autoPID.Compute();

  this->lastComputed = map(output, MIN_PID_OUTPUT_LIMIT, MAX_PID_OUTPUT_LIMIT,
                           this->maxOutput, this->minOutput);
}