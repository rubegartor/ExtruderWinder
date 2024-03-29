#include <Commons/Commons.h>
#include <PID/PIDPuller.h>
#include <PID_v1.h>

PID pullerPID(0, 0, 0, 0, 0, 0, DIRECT);

void PIDPuller::init() {
  PID pid(&this->input, &this->output, &this->setPoint, this->Kp, this->Ki,
          this->Kd, DIRECT);

  pullerPID = pid;

  this->setPoint = filamentDiameter;
  this->minOutput =
      pref.getUInt(MIN_PULLER_SPEED_PREF, MIN_PULLER_SPEED_DEFAULT);
  this->maxOutput =
      pref.getUInt(MAX_PULLER_SPEED_PREF, MAX_PULLER_SPEED_DEFAULT);

  pullerPID.SetTunings(this->Kp, this->Ki, this->Kd);
  pullerPID.SetMode(AUTOMATIC);
}

double PIDPuller::getSetPoint() { return this->setPoint; }

void PIDPuller::updateSetPoint(float setPoint) {
  this->setPoint = setPoint;
}

uint16_t PIDPuller::getMinPullerSpeed() {
  return pref.getUInt(MIN_PULLER_SPEED_PREF);
}

uint16_t PIDPuller::getMaxPullerSpeed() {
  return pref.getUInt(MAX_PULLER_SPEED_PREF);
}

void PIDPuller::updateMinPullerSpeed(uint16_t speed) {
  pref.putUInt(MIN_PULLER_SPEED_PREF, speed);
  this->minOutput = speed;
}

void PIDPuller::updateMaxPullerSpeed(uint16_t speed) {
  pref.putUInt(MAX_PULLER_SPEED_PREF, speed);
  this->maxOutput = speed;
}

uint16_t PIDPuller::computeSpeed() {
  this->input = measuring.lastRead;

  double gap = abs(this->setPoint - this->input);

  if (gap > PID_AGGRESSIVE_GAP) {
    pullerPID.SetTunings(this->aggKp, this->aggKi, this->aggKd);
  } else {
    pullerPID.SetTunings(this->Kp, this->Ki, this->Kd);
  }

  this->doCompute();

  return this->lastComputed;
}

void PIDPuller::doCompute() {
  pullerPID.Compute();

  this->lastComputed =
      map(this->output, MIN_PID_PULLER_OUTPUT_LIMIT,
          MAX_PID_PULLER_OUTPUT_LIMIT, this->maxOutput, this->minOutput);
}