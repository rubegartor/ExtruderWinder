#pragma once

#include <Arduino.h>

#define MIN_PID_PULLER_OUTPUT_LIMIT 0
#define MAX_PID_PULLER_OUTPUT_LIMIT 255
#define PID_AGGRESSIVE_GAP 0.18f

class PIDPuller {
 private:
  double setPoint, input, output;
  double aggKp = 5, aggKi = 25, aggKd = 0.05;
  double Kp = 2, Ki = 12.5, Kd = 1;
  uint16_t lastComputed;

  void doCompute(float input);

 public:
  bool stabilized;
  uint16_t minOutput;
  uint16_t maxOutput;

  void init();

  void updateSetPoint(float setPoint);

  uint16_t computeSpeed();
};