#pragma once

#include <Arduino.h>
#include <Commons/Commons.h>

#define DEFAULT_DEVIATION 0.10f

class Measuring {
 public:
  bool state = true;
  float readValueSum = 0.00f;
  uint32_t readValueNum = 0;
  float lastRead = 0.00f;
  float minRange = filamentDiameter - DEFAULT_DEVIATION;
  float maxRange = filamentDiameter + DEFAULT_DEVIATION;
  float minRead = 0.00f;
  float maxRead = 0.00f;

  void setMinRange(float value);

  void setMaxRange(float value);

  bool checkInRange();

  float read();

  float average();

  void reset();
};