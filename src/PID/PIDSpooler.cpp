#include <Commons/Commons.h>
#include <PID/PIDSpooler.h>
#include <Winder/Winder.h>
#include <PID_v1.h>

double setPoint1, input1, output1;
double Kp1 = 3, Ki1 = 1.5, Kd1 = 0.05;

PID autoPID1(&input1, &output1, &setPoint1, Kp1, Ki1, Kd1, DIRECT);

void PIDSpooler::init() {
  setPoint1 = 125;
  this->minOutput = 0;
  this->maxOutput = SPOOL_MAX_SPEED / 2;

  autoPID1.SetMode(AUTOMATIC);
}

uint16_t PIDSpooler::computeSpeed() {
  input1 = actualDistance;

  this->doCompute(input1);

  return this->lastComputed;
}

void PIDSpooler::doCompute(float input) {
  autoPID1.Compute();

  this->lastComputed = map(output1, MIN_PID_OUTPUT_LIMIT, MAX_PID_OUTPUT_LIMIT,
                           this->maxOutput, this->minOutput);
}