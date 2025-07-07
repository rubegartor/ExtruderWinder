#pragma once

#include <Arduino.h>
#include <NewPing.h>

#define TENSIONER_TRIGGER_PIN 20
#define TENSIONER_ECHO_PIN 21

#define TENSIONER_SETPOINT 80
#define TENSIONER_MAX_DISTANCE 30

class Tensioner {
  private:
    unsigned long tensioner_loop_last_millis = 0;
  public:
    uint16_t distance;

    void setup();
    void loop(unsigned long interval = 0);
};