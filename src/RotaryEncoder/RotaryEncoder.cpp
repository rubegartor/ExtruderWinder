#include <Arduino.h>
#include <RotaryEncoder/RotaryEncoder.h>
#include <Commons/Commons.h>
#include <AiEsp32RotaryEncoder.h>

#define minValue -10000000
#define maxValue 10000000

AiEsp32RotaryEncoder PhysREncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

void IRAM_ATTR readEncoderISR() {
  PhysREncoder.readEncoder_ISR();
}

void REncoder::init()
{
    PhysREncoder.begin();
    PhysREncoder.setup(readEncoderISR);
    PhysREncoder.setBoundaries(minValue, maxValue, false); // minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    PhysREncoder.setAcceleration(RORATY_ENCODER_ACCELERATION);
}

long REncoder::value()
{
    return PhysREncoder.readEncoder();
}

bool REncoder::clicked()
{
    return PhysREncoder.isEncoderButtonClicked();
}

bool REncoder::changed()
{
    return PhysREncoder.encoderChanged();
}