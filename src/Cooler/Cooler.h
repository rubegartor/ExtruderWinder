#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define TEMP_SENSOR_PIN 1

#define TEMP_DEFAULT_PREF 6
#define TEMP_PREF "temp"

#define TEMP_THRESHOLD 2

class Cooler {
 private:
  bool isRefrigerationNeeded();
 public:
  void init();

  void refresh();

  float read();
};