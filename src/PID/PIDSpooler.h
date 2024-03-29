#pragma once

#include <Arduino.h>

#define MIN_PID_SPOOLER_OUTPUT_LIMIT 0
#define MAX_PID_SPOOLER_OUTPUT_LIMIT 255

#define SETPOINT_PID_SPOOLER 100

class PIDSpooler {
 private:
  uint16_t lastComputed;
  double Kp = 1.2;
  double Ki = 1.5;
  double Kd = 0.03;
  double setPoint, input, output;

  void doCompute();

 public:
  uint16_t minOutput;
  uint16_t maxOutput;

  void init();

  uint16_t computeSpeed();
};