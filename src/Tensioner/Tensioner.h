#pragma once

#include <Arduino.h>

#define GPIO1_INTERRUPT_PIN 39
#define SHUTDOWN_PIN 14


class Tensioner {
 public:
  uint16_t lastRead;

  void init();

  uint16_t IRAM_ATTR getDistance();
};