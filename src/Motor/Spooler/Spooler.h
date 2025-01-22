#pragma once

#include <TMC51X0.hpp>
#include <QuickPID.h>

#define SPOOLER_CS_PIN 8

#define MIN_PID_SPOOLER_OUTPUT_LIMIT 0
#define MAX_PID_SPOOLER_OUTPUT_LIMIT 255

#define SETPOINT_PID_SPOOLER 100

#define SPOOLER_MAX_SPEED 220

#define SPOOLER_GEARBOX_RATIO 5
#define SPOOLER_ONE_REV_STEPS (51200 * SPOOLER_GEARBOX_RATIO)


class Spooler {
 private:
  TMC51X0 spooler;
  QuickPID pid;

  float setPoint, input, output;
  float Kp = 0.8, Ki = 1, Kd = 0.03;
  unsigned long spooler_loop_last_millis = 0;

  void setupPID();
  void setupDriver();
 public:
  uint16_t minOutput;
  uint16_t maxOutput;
  uint32_t revs;
  uint16_t speed;

  void setup();
  void loop(unsigned long interval = 0);
};