#pragma once

#include <Arduino.h>

#define MIN_PID_OUTPUT_LIMIT 0
#define MAX_PID_OUTPUT_LIMIT 255
#define PID_AGGRESSIVE_GAP 0.18f

class PIDPuller {
 private:
  uint16_t lastComputed;

  void doCompute(float input);
 public:
  bool stabilized;
  uint16_t minOutput;
  uint16_t maxOutput;

  void init();

  uint16_t computeSpeed();
};