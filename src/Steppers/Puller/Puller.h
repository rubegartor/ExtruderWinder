#pragma once

#include <AccelStepper.h>
#include <TMCStepper.h>

#define PULLER_MICROSTEPS 16
#define PULLER_ONE_REV_STEPS (STEPPER_DEF_STEPS * PULLER_MICROSTEPS)

#define PULLER_CS_PIN 14

#define PULLER_STEP_PIN 4
#define PULLER_MAX_SPEED 13200
#define DEFAULT_PULLER_SPEED 1500
#define PULLER_DIAM 24.5f  // In mm

class Puller {
 private:
  void configDriver();

 public:
  long totalRevs;
  uint16_t speed = DEFAULT_PULLER_SPEED;

  void init();

  void run();
};