#pragma once

#include <TMC51X0.hpp>
#include "Commons/TimedComponent.h"
#include <QuickPID.h>

#define PULLER_DIAM 24.5f  // In mm

#define PID_AGGRESSIVE_GAP 0.5f

#define MIN_PID_PULLER_OUTPUT_LIMIT 0
#define MAX_PID_PULLER_OUTPUT_LIMIT 255

#define MIN_PULLER_PREF "pMinSpeed"
#define MAX_PULLER_PREF "pMaxSpeed"

#define MIN_PULLER_SPEED_DEFAULT 1750
#define MAX_PULLER_SPEED_DEFAULT 8000

#define PULLER_ONE_REV_STEPS 51200

class Puller : public TimedComponent {
private:
  TMC51X0 motor;

  QuickPID pid;
  
  int32_t lastPosition;
  int32_t revolutionCount;

  float setPoint, input, output;
  float aggKp = 5, aggKi = 25, aggKd = 0.05;
  float Kp = 2, Ki = 12.5, Kd = 1;
  unsigned long puller_loop_last_millis = 0;

  void setupPID();
  void setupDriver();
    
public:
  int minOutput;
  int maxOutput;
  uint16_t speed = MIN_PULLER_SPEED_DEFAULT;

  Puller() : TimedComponent() {}

  void setup();
  void execute() override;

  void updateSetPoint(float value);
  void updateMinOutput(uint32_t value);
  void updateMaxOutput(uint32_t value);
  
  int32_t getRevolutionCount() { return revolutionCount; }
  void resetRevolutionCount() { revolutionCount = 0; }
};
