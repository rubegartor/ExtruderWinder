#pragma once

#include <AccelStepper.h>

#define PULLER_STEP_PIN 25
#define PULLER_MAX_SPEED 13200
#define DEFAULT_PULLER_SPEED 1500
#define PULLER_DIAM 24.5f  // In mm

class Puller {
 public:
  long totalRevs;
  uint16_t speed = DEFAULT_PULLER_SPEED;

  void init();

  void run();
};