#pragma once

#include <Arduino.h>

#define MEASUREMENT_REQ_PIN 24
#define MEASUREMENT_CLK_PIN 23
#define MEASUREMENT_DATA_PIN 22

#define MITUTOYO_CLK_DEBOUNCE 220

#define ERROR_DETECTION_ENABLE_THRESHOLD 0.15f
#define ERROR_DETECTION_THRESHOLD 1.00f

#define MEASUREMENT_LIMIT 10.60f

class Measurement {
  // private:
  //   bool errorDetectionEnabled = false;
  public:
    float lastRead = 0.00f;
    float minRead = 0.00f;
    float maxRead = 0.00f;

    void setup();
    void loop();
    void reset();
};