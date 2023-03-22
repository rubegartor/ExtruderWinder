#pragma once

#include <Arduino.h>
#include <Commons/Commons.h>

#define MEASURING_MODE_PREF "measuringMode"

enum MeasuringMode {
  measuringAutoMode,
  measuringManualMode
};

class Measuring {
 public:
  MeasuringMode mode = measuringManualMode;
  float readValueSum = 0.00f;
  uint32_t readValueNum = 0;
  float lastRead = 0.00f;
  float minRead = 0.00f;
  float maxRead = 0.00f;

  void init();

  float read();

  float average();

  void reset();

  String measuringModeString();
};