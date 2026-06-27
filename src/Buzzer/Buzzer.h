#pragma once

#include <Arduino.h>
#include "Commons/pins.h"
#include "mbed.h"

enum BuzzerBeep {
  BEEP_SLOW = 1000,
  BEEP_MEDIUM = 500,
  BEEP_FAST = 250,
  BEEP_ULTRA_FAST = 125
};

class Buzzer {
  public:
    static Buzzer& getInstance() {
      static Buzzer instance;
      return instance;
    }

    Buzzer(const Buzzer&)            = delete;
    Buzzer& operator=(const Buzzer&) = delete;

  void beep(BuzzerBeep speed) {
    if (!isActive) {
      pwm->period_ms(speed);
      pwm->pulsewidth_ms(speed / 2);

      isActive = true;
    }
  }

  void stop() {
    pwm->write(0.0f);
    isActive = false;
  }

  private:
    mbed::PwmOut* pwm;
    bool isActive;

    Buzzer() {
      pwm = new mbed::PwmOut(digitalPinToPinName(BUZZER_PIN));
      this->stop();
    }
};
