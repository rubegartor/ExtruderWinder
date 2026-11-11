#pragma once

#include <Arduino.h>
#include <NewPing.h>
#include "Commons/TimedComponent.h"

#define TENSIONER_MIN_HARD_LIMIT 50
#define TENSIONER_SETPOINT 140
#define TENSIONER_MAX_DISTANCE 300
#define TENSIONER_START_THRESHOLD 170
// Ecos perdidos seguidos (x100 ms) antes de dar el sensor por caído y parar el spooler.
#define TENSIONER_MAX_MISSED_ECHOES 20

class Tensioner : public TimedComponent {
  public:
    static Tensioner& getInstance() {
      static Tensioner instance;
      return instance;
    }

    Tensioner(const Tensioner&)            = delete;
    Tensioner& operator=(const Tensioner&) = delete;

    uint16_t distance;
    uint8_t  missedEchoes = 0;

    void setup(unsigned long interval);
    void execute() override;

  private:
    Tensioner() : TimedComponent() {}
};