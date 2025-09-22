#pragma once

#include <TMC51X0.hpp>
#include "Commons/TimedComponent.h"
#include <QuickPID.h>

#define SPOOLER_MAX_SPEED 65
#define MIN_PID_SPOOLER_OUTPUT_LIMIT 0
#define MAX_PID_SPOOLER_OUTPUT_LIMIT 255

class Spooler : public TimedComponent {
private:
  TMC51X0 motor;
  QuickPID pid;
  
  int32_t lastPosition;
  int32_t revolutionCount;
  bool thresholdReached;

  float setPoint, input, output;
  float Kp = 0.8, Ki = 0.5, Kd = 0.1;

  void setupPID();
  void setupDriver();

  void reinit();
  bool enabled();
    
public:
  Spooler() : TimedComponent() {}

  uint16_t minOutput;
  uint16_t maxOutput;

  uint16_t speed;

  void setup();
  void execute() override;

  int32_t getRevolutionCount() const { return revolutionCount; }
};
