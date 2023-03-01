#pragma once

#include <Arduino.h>

class Tensioner
{
public:
    void init();

    IRAM_ATTR uint16_t getDistance();
};