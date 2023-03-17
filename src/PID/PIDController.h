#pragma once

#include <Arduino.h>

class PIDController {
 public:
  uint16_t minOutput;
  uint16_t maxOutput;

  void init();

  uint16_t computeSpeed();
};