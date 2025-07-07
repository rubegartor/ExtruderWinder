#include "Measurement.h"
#include "Commons/Commons.h"
#include "Screen/components/general.h"

enum class ReadState {
  IDLE,
  READING,
  DONE
};

volatile uint8_t fullSPCData[52];
volatile uint8_t spcdata[13];
volatile ReadState spcState = ReadState::IDLE;
volatile uint8_t bitBuffer = 0;  // almacena temporalmente 4 bits
volatile uint8_t totalBits = 0;  // de 0-51 (13 * 4)
volatile unsigned long lastMicros = 0;

void readBitISR() {
  unsigned long now = micros();
  if (now - lastMicros < MITUTOYO_CLK_DEBOUNCE) return;
  lastMicros = now;

  if (spcState != ReadState::READING || totalBits >= 52) return;

  // Desplaza el buffer y añade el nuevo bit (MSB first)
  bitBuffer = (bitBuffer >> 1) | (digitalRead(MEASUREMENT_DATA_PIN) << 3);

  fullSPCData[totalBits] = bitBuffer;
  totalBits++;

  if (totalBits % 4 == 0) {
    uint8_t idx = (totalBits / 4) - 1;
    spcdata[idx] = bitBuffer;

    // Si la cabezera no es 0xF, reinicia la lectura (sincronización)
    if (idx < 4 && bitBuffer != 0xF) {
      totalBits = 0;
      bitBuffer = 0;
      return;
    }

    bitBuffer = 0;

    if (totalBits == 52) {
      digitalWrite(MEASUREMENT_REQ_PIN, HIGH);
      spcState = ReadState::DONE;
    }
  }
}

bool isValidSPCFrame(const volatile uint8_t* data) {
  // Cabecera: d1–d4 deben ser 0xF
  for (uint8_t i = 0; i < 4; i++) {
    if (data[i] != 0xF) return false;
  }
  // Signo: d5 debe ser 0x0 o 0x8
  if (data[4] != 0x0 && data[4] != 0x8) return false;
  // Dígitos: d6–d11 deben ser 0–9
  for (uint8_t i = 5; i <= 10; i++) {
    if (data[i] > 9) return false;
  }
  // Punto decimal: d12 debe ser 2–5
  if (data[11] < 2 || data[11] > 5) return false;
  // Unidad: d13 debe ser 0 o 1
  if (data[12] > 1) return false;
  return true;
}

void Measurement::setup() {
  memset((void*)spcdata, 0, sizeof(spcdata));
  memset((void*)fullSPCData, 0, sizeof(fullSPCData));

  attachInterrupt(digitalPinToInterrupt(MEASUREMENT_CLK_PIN), readBitISR, FALLING);
  pinMode(MEASUREMENT_CLK_PIN, INPUT_PULLDOWN); // Hardware pullup & software pulldown WTF (bug: https://github.com/arduino/ArduinoCore-mbed/issues/780)
  pinMode(MEASUREMENT_DATA_PIN, INPUT_PULLUP);
  pinMode(MEASUREMENT_REQ_PIN, OUTPUT);
  digitalWrite(MEASUREMENT_REQ_PIN, HIGH);
}

void Measurement::loop() {
  if (spcState == ReadState::IDLE) {
    totalBits = 0;
    bitBuffer = 0;
    memset((void*)spcdata, 0, sizeof(spcdata));
    memset((void*)fullSPCData, 0, sizeof(fullSPCData));
    spcState = ReadState::READING;
    digitalWrite(MEASUREMENT_REQ_PIN, LOW);
  }

  if (spcState != ReadState::DONE) return;

  // Validar frame antes de procesar
  if (!isValidSPCFrame(spcdata)) {
    spcState = ReadState::IDLE;
    Serial.println("Frame SPC inválido:");
    for (uint8_t i = 0; i < 13; i++) {
      Serial.print(spcdata[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    return;
  }

  float value = 0.0f;
  for (uint8_t i = 5; i <= 10; i++) {
    value = value * 10 + spcdata[i];
  }

  value /= pow(10, spcdata[11]);

  if (spcdata[4] & 0x8) {
    value = -value;
  }

  if (fabs(value) > MEASUREMENT_LIMIT) {
    spcState = ReadState::IDLE;
    return;
  }

  this->lastRead = value;
  spcState = ReadState::IDLE;

  if (this->lastRead < this->minRead) {
    this->minRead = this->lastRead;
  } else if (this->lastRead > this->maxRead) {
    this->maxRead = this->lastRead;
  }

  addChartValue(static_cast<int32_t>(this->lastRead * 100));
}

void Measurement::reset() {
  this->minRead = this->maxRead = this->lastRead;
  puller.revs = 0;
}
