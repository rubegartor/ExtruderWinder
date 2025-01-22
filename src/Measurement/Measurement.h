#pragma once

#include <Arduino.h>

#define CLOCK_PIN 5
#define DATA_PIN 4

#define MEASUREMENT_BITS 24
#define DATA_OUTPUT_RATE_MS 80

class Measurement {
  private:
    unsigned long measurement_decode_last_millis = 0;

    void decode();
  public:
    float lastRead = 0.00f;
    float minRead = 0.00f;
    float maxRead = 0.00f;

    void setup();
    void loop();

    void reset();
};