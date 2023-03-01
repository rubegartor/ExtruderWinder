#pragma once

#include <Arduino.h>

class ESPNowConnection
{
public:
  void init();

  bool send();
};

void setSpoolSpeed(uint16_t speed);