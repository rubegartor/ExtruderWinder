#pragma once

#include <Arduino.h>
#include "Commons/TimedComponent.h"

#define MITUTOYO_CLK_DEBOUNCE 280

#define MEASUREMENT_LIMIT 10.60f

class Measurement : public TimedComponent {    
public:
    float lastRead = 0.00f;
    float minRead = 0.00f;
    float maxRead = 0.00f;
    unsigned long lastUpdateTime = 0;

    Measurement() : TimedComponent() {}

    void setup();
    void execute() override;
    void reset();

private:
    float previousValidRead = 0.00f;
    float suspectedRead = 0.00f;
    bool hasSuspectedRead = false;
    
    bool shouldFilterReading(float newValue);
    void processValidReading(float value);
};