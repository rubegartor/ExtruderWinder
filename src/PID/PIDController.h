#pragma once

#include <Arduino.h>

#define MAX_VALID_DEVIATION 0.08f

class PIDController {
 private:
  uint16_t lastComputedDiameter;
  uint16_t lastComputed;

  bool isValidToCompute(float input);

 public:
  bool stabilized;
  uint16_t minOutput;
  uint16_t maxOutput;

  void init();

  uint16_t computeSpeed();
};