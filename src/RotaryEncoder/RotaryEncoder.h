#pragma once

#include <Arduino.h>

class REncoder
{
public:
    void init();

    long value();

    bool clicked();

    bool changed();
};