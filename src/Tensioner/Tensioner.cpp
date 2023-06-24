#include <Commons/Commons.h>
#include <Tensioner/Tensioner.h>
#include <VL53L0X.h>

VL53L0X lox;

void Tensioner::init() {
  lox.setTimeout(250);

  if (!lox.init()) {
    Serial.println(F("Failed to boot VL53L0X!"));
    lcdMenu.println("Failed boot VL53L0X", 0, true);
    while (1)
      ;
  }
}

uint16_t IRAM_ATTR Tensioner::getDistance() {
  this->lastRead = lox.readRangeSingleMillimeters();

  return this->lastRead;
}