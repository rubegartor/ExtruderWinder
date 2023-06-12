#include <Adafruit_VL53L0X.h>
#include <Commons/Commons.h>
#include <Tensioner/Tensioner.h>

volatile byte VL53LOX_State = LOW;

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

void IRAM_ATTR _VL53LOXISR() {
  VL53LOX_State = digitalRead(GPIO1_INTERRUPT_PIN);
}

void Tensioner::init() {
  pinMode(SHUTDOWN_PIN, INPUT_PULLUP);
  pinMode(GPIO1_INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(GPIO1_INTERRUPT_PIN), _VL53LOXISR,
                  CHANGE);

  while (!lox.begin()) {
    digitalWrite(SHUTDOWN_PIN, LOW);
    delay(100);
    lcdMenu.println("Restarting VL53L0X", 0, true);
    digitalWrite(SHUTDOWN_PIN, HIGH);
    delay(100);
  }

  lox.setGpioConfig(VL53L0X_DEVICEMODE_CONTINUOUS_RANGING,
                    VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY,
                    VL53L0X_INTERRUPTPOLARITY_LOW);

  lox.setDeviceMode(VL53L0X_DEVICEMODE_CONTINUOUS_RANGING, false);

  lox.startMeasurement();
}

uint16_t IRAM_ATTR Tensioner::getDistance() {
  if (VL53LOX_State == LOW) {
    VL53L0X_RangingMeasurementData_t measure;
    lox.getRangingMeasurement(&measure, false);

    if (measure.RangeStatus != VL53L0X_STATE_RUNNING) {
      this->lastRead = measure.RangeMilliMeter;
    }

    lox.clearInterruptMask(false);
  }

  return this->lastRead;
}