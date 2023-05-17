#pragma once

#include <Arduino.h>
#include <Commons/Commons.h>

#define AUTOSTOP_ENABLE_THRESHOLD 0.15

#define MEASURING_MODE_PREF "measuringMode"

#define AUTOSTOP_THRESHOLD_PREF "autoStopThr"
#define AUTOSTOP_THRESHOLD_DEFAULT 0.2f

enum MeasuringMode { measuringAutoMode, measuringManualMode };

class Measuring {
 private:
  bool autoStopEnabled = false;

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