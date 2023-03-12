#pragma once

#include <Arduino.h>

class Tensioner {
 public:
  void init();

  uint16_t IRAM_ATTR getDistance();
};