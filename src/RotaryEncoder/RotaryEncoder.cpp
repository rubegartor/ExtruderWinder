#include <AiEsp32RotaryEncoder.h>
#include <Arduino.h>
#include <Commons/Commons.h>
#include <RotaryEncoder/RotaryEncoder.h>

#define minValue 1
#define maxValue 1000000

AiEsp32RotaryEncoder PhysREncoder = AiEsp32RotaryEncoder(
    ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN,
    ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

void IRAM_ATTR readEncoderISR() { PhysREncoder.readEncoder_ISR(); }

void REncoder::init() {
  PhysREncoder.begin();
  PhysREncoder.setBoundaries(
      minValue, maxValue,
      false);  // minValue, maxValue, circleValues true|false (when max go to
               // min and vice versa)
  PhysREncoder.setEncoderValue(500000);
  PhysREncoder.setAcceleration(RORATY_ENCODER_ACCELERATION);

  PhysREncoder.setup(readEncoderISR);

  REncoder::lastReadValue = -1;
}

bool REncoder::clicked() { return PhysREncoder.isEncoderButtonClicked(); }

bool REncoder::changed() {
  bool changed = PhysREncoder.encoderChanged();

  if (changed) {
    long value = PhysREncoder.readEncoder();

    this->direction = value >= this->lastReadValue ? increased : decreased;

    if (value != this->lastReadValue) {
      this->lastReadValue = value;
    }
  }

  return changed;
}