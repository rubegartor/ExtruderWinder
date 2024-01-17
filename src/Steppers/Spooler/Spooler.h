#pragma once

#include <AccelStepper.h>
#include <TMCStepper.h>

#define SPOOL_STEP_PIN 5
#define SPOOL_CS_PIN 14
#define SPOOL_MAX_SPEED 7000

#define SPOOL_MICROSTEPS 8
#define SPOOL_MOTOR_RATIO 5.16
#define SPOOL_ONE_REV_STEPS \
  ((STEPPER_DEF_STEPS * SPOOL_MICROSTEPS) * SPOOL_MOTOR_RATIO)

class Spooler {
 private:
  void configDriver();

 public:
  long totalRevs;
  uint16_t speed;

  void init();

  void run();

  bool isEnabled(bool restartIfDisabled = false);
};