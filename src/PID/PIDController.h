#pragma once

#include <Arduino.h>

class PIDController {
 public:
  void init();

  void compute();
};