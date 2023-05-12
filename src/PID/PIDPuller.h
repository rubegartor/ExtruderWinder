#pragma once

#include <Arduino.h>

#define MIN_PID_PULLER_OUTPUT_LIMIT 0
#define MAX_PID_PULLER_OUTPUT_LIMIT 255
#define PID_AGGRESSIVE_GAP 0.5f

#define MIN_PULLER_SPEED_DEFAULT 900
#define MIN_PULLER_SPEED_PREF "minPullerSpeed"

#define MAX_PULLER_SPEED_DEFAULT 3000
#define MAX_PULLER_SPEED_PREF "maxPullerSpeed"

#define PID_KI_PREF "pullerPID_Ki"
#define PID_KI_DEFAULT 12.5

class PIDPuller {
 private:
  double setPoint, input, output;
  double aggKp = 5, aggKi = 25, aggKd = 0.05;
  double Kp = 2, Ki = PID_KI_DEFAULT, Kd = 1;
  uint16_t lastComputed;

  void doCompute(float input);

 public:
  uint16_t minOutput;
  uint16_t maxOutput;

  void init();

  double getSetPoint();

  void updateSetPoint(float setPoint);

  void updateMinPullerSpeed(uint16_t speed);

  void updateMaxPullerSpeed(uint16_t speed);

  void updateKi(double ki);

  uint16_t computeSpeed();
};