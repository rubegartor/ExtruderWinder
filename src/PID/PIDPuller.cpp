#include <Commons/Commons.h>
#include <PID/PIDPuller.h>
#include <PID_v1.h>

PID pullerPID(0, 0, 0, 0, 0, 0, DIRECT);

void PIDPuller::init() {
  PID pid(&this->input, &this->output, &this->setPoint, this->Kp, this->Ki,
          this->Kd, DIRECT);

  pullerPID = pid;

  this->setPoint = filamentDiameter;
  this->minOutput = 950;
  this->maxOutput = 2200;

  pullerPID.SetMode(AUTOMATIC);
}

void PIDPuller::updateSetPoint(float setPoint) {
  this->setPoint = setPoint;
}

uint16_t PIDPuller::computeSpeed() {
  this->input = measuring.lastRead;

  double gap = abs(this->setPoint - this->input);

  if (gap > PID_AGGRESSIVE_GAP) {
    pullerPID.SetTunings(this->aggKp, this->aggKi, this->aggKd);
  } else {
    pullerPID.SetTunings(this->Kp, this->Ki, this->Kd);
  }

  this->doCompute(this->input);

  return this->lastComputed;
}

void PIDPuller::doCompute(float input) {
  pullerPID.Compute();

  this->lastComputed =
      map(this->output, MIN_PID_PULLER_OUTPUT_LIMIT, MAX_PID_PULLER_OUTPUT_LIMIT,
          this->maxOutput, this->minOutput);
}