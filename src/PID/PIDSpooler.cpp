#include <Commons/Commons.h>
#include <PID/PIDSpooler.h>
#include <PID_v1.h>

PID spoolerPID(0, 0, 0, 0, 0, 0, DIRECT);

void PIDSpooler::init() {
  PID pid(&this->input, &this->output, &this->setPoint, this->Kp, this->Ki,
          this->Kd, DIRECT);

  spoolerPID = pid;

  this->setPoint = SETPOINT_PID_SPOOLER;
  this->minOutput = 0;
  this->maxOutput = SPOOL_MAX_SPEED;

  spoolerPID.SetMode(AUTOMATIC);
}

uint16_t PIDSpooler::computeSpeed() {
  this->input = tensioner.lastRead;

  this->doCompute();

  return this->lastComputed;
}

void PIDSpooler::doCompute() {
  spoolerPID.Compute();

  this->lastComputed =
      map(this->output, MIN_PID_SPOOLER_OUTPUT_LIMIT,
          MAX_PID_SPOOLER_OUTPUT_LIMIT, this->maxOutput, this->minOutput);
}