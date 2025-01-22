#include "Measurement.h" 

// https://gist.github.com/kesor/3577811486a4032baed0549bbc61c4df

volatile unsigned long lastClock = 0;
volatile bool lastClockState = LOW;

volatile byte dataBits[MEASUREMENT_BITS];
volatile int bitCount = 0;

const unsigned long IDLE_TIME = 100000;
const unsigned long DEBOUNCE_US = 20;

void _CLK_ISR() {
  unsigned long currentUS = micros();

  if (currentUS - lastClock < DEBOUNCE_US)
    return;

  if (currentUS - lastClock > IDLE_TIME)
    bitCount = 0;

  bool state = digitalRead(CLOCK_PIN);

  if (lastClockState == LOW && state == HIGH && bitCount == MEASUREMENT_BITS) {
    dataBits[bitCount - 1] = digitalRead(DATA_PIN);
    bitCount++;
  }

  if (lastClockState == HIGH && state == LOW && bitCount < MEASUREMENT_BITS) {
    dataBits[bitCount] = digitalRead(DATA_PIN);
    bitCount++;
  }

  lastClockState = state;
  lastClock = currentUS;
}

void Measurement::decode() {
  int8_t sign = 1;
  int16_t value = 0;

  if (dataBits[0] != 1)
    return;

  for (int i = 0; i <= 20; i++)
    if (dataBits[i + 1])
      value |= 1 << i;

  sign = (dataBits[21] == 1) ? -1 : 1;

  unsigned long currentMillis = millis();
  if (currentMillis - measurement_decode_last_millis >= DATA_OUTPUT_RATE_MS) {
    measurement_decode_last_millis = currentMillis;
    this->lastRead = (value * sign) / 100.0;
  }
}

void Measurement::setup() {
  pinMode(CLOCK_PIN, INPUT);
  pinMode(DATA_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLOCK_PIN), _CLK_ISR, CHANGE);
}

void Measurement::loop() {
  if (bitCount > MEASUREMENT_BITS) decode();

  if (this->lastRead < this->minRead) {
    this->minRead = this->lastRead;
  } else if (this->lastRead > this->maxRead) {
    this->maxRead = this->lastRead;
  }
}

void Measurement::reset() {
  this->minRead = this->lastRead;
  this->maxRead = this->lastRead;
  this->lastRead = 0;
}