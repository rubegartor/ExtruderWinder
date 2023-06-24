#pragma once

#include <AccelStepper.h>
#include <TMCStepper.h>

#define SPOOL_STEP_PIN 32
#define SPOOL_CS_PIN 15
#define SPOOL_MAX_SPEED 3500

class Spooler {
 public:
  long totalRevs;
  uint16_t speed;

  void init();

  void configDriver();

  void run();
};