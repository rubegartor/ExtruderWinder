#pragma once

#include <Arduino.h>
#include <NewPing.h>
#include "Commons/TimedComponent.h"

#define TENSIONER_MIN_HARD_LIMIT 50
#define TENSIONER_SETPOINT 140
#define TENSIONER_MAX_DISTANCE 300
#define TENSIONER_START_THRESHOLD 170

class Tensioner : public TimedComponent {
  public:
    static Tensioner& getInstance() {
      static Tensioner instance;
      return instance;
    }

    Tensioner(const Tensioner&)            = delete;
    Tensioner& operator=(const Tensioner&) = delete;

    uint16_t distance;

    void setup();
    void execute() override;

  private:
    Tensioner() : TimedComponent() {}
};