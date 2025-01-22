#pragma once

#include <Arduino.h>
#include <NewPing.h>

#define TENSIONER_TRIGGER_PIN 6
#define TENSIONER_ECHO_PIN 7

class Tensioner {
  private:
    unsigned long tensioner_loop_last_millis = 0;
  public:
    float distance;

    void setup();
    void loop(unsigned long interval = 0);
};