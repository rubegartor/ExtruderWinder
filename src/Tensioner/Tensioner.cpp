#include <Tensioner/Tensioner.h>
#include <VL53L0X.h>

VL53L0X lox;

void Tensioner::init()
{
    if (!lox.init())
    {
        Serial.println(F("Failed to boot VL53L0X!"));
        while (1);
    }

    lox.setTimeout(250);
}

IRAM_ATTR uint16_t Tensioner::getDistance()
{
    return lox.readRangeSingleMillimeters();
}