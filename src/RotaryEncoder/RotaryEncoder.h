#pragma once

#include <Arduino.h>

#define ROTARY_ENCODER_A_PIN 7        // DT
#define ROTARY_ENCODER_B_PIN 6        // CLK
#define ROTARY_ENCODER_BUTTON_PIN 15  // SW
#define ROTARY_ENCODER_STEPS 4
#define ROTARY_ENCODER_VCC_PIN -1
#define RORATY_ENCODER_ACCELERATION 120

enum RotaryDirection { decreased, increased };

class REncoder {
 public:
  long lastReadValue;

  RotaryDirection direction;

  void init();

  bool clicked();

  bool changed();
};