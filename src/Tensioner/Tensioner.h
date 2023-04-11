#pragma once

#include <Arduino.h>

class Tensioner {
 public:
  uint16_t lastRead;

  void init();

  uint16_t IRAM_ATTR getDistance();
};