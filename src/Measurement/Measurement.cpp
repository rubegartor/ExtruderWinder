#include "Measurement/Measurement.h"
#include "Commons/pins.h"

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

  // Verificar que los pines sean válidos antes de leer
  if (MEASUREMENT_DATA_PIN >= NUM_DIGITAL_PINS) return;

  // Desplaza el buffer y añade el nuevo bit (MSB first)
  bitBuffer = (bitBuffer >> 1) | (digitalRead(MEASUREMENT_DATA_PIN) << 3);

  fullSPCData[totalBits] = bitBuffer;
  totalBits++;

  if (totalBits % 4 == 0) {
    uint8_t idx = (totalBits / 4) - 1;
    if (idx >= 13) return; // Protección adicional contra desbordamiento
    
    spcdata[idx] = bitBuffer;

    // Si la cabezera no es 0xF, reinicia la lectura (sincronización)
    if (idx < 4 && bitBuffer != 0xF) {
      totalBits = 0;
      bitBuffer = 0;
      return;
    }

    bitBuffer = 0;

    if (totalBits == 52) {
      if (MEASUREMENT_REQ_PIN < NUM_DIGITAL_PINS) {
        digitalWrite(MEASUREMENT_REQ_PIN, HIGH);
      }
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

bool Measurement::isFirstDigitError(float newValue, float previousValue) {
  // Si no tenemos una lectura previa válida, aceptamos la nueva
  if (previousValue == 0.0f) return false;
  
  // Si el cambio es mayor de 1.0f, consideramos que puede ser un error del primer dígito
  float diff = fabs(newValue - previousValue);
  return diff > 1.0f;
}

void Measurement::setup() {
  memset((void*)spcdata, 0, sizeof(spcdata));
  memset((void*)fullSPCData, 0, sizeof(fullSPCData));

  attachInterrupt(digitalPinToInterrupt(MEASUREMENT_CLK_PIN), readBitISR, FALLING);
  pinMode(MEASUREMENT_CLK_PIN, INPUT_PULLDOWN);
  pinMode(MEASUREMENT_DATA_PIN, INPUT_PULLUP);
  pinMode(MEASUREMENT_REQ_PIN, OUTPUT);
  digitalWrite(MEASUREMENT_REQ_PIN, HIGH);
}

void Measurement::execute() {
  if (spcState == ReadState::IDLE) {
    totalBits = 0;
    bitBuffer = 0;
    memset((void*)spcdata, 0, sizeof(spcdata));
    memset((void*)fullSPCData, 0, sizeof(fullSPCData));
    spcState = ReadState::READING;
    digitalWrite(MEASUREMENT_REQ_PIN, LOW);
  }

  if (spcState != ReadState::DONE) return;

  if (!isValidSPCFrame(spcdata)) {
    spcState = ReadState::IDLE;
    return;
  }

  float value = 0.0f;
  for (uint8_t i = 5; i <= 10; i++) {
    if (spcdata[i] > 9) { // Verificación adicional de seguridad
      spcState = ReadState::IDLE;
      return;
    }
    value = value * 10 + spcdata[i];
  }

  // Verificar que el divisor sea válido antes de la división
  uint8_t decimalPlaces = spcdata[11];
  if (decimalPlaces < 2 || decimalPlaces > 5) {
    spcState = ReadState::IDLE;
    return;
  }

  value /= pow(10, decimalPlaces);

  if (spcdata[4] & 0x8) {
    value = -value;
  }

  // Verificación adicional contra valores NaN e infinitos
  if (!isfinite(value) || fabs(value) > MEASUREMENT_LIMIT) {
    spcState = ReadState::IDLE;
    return;
  }

  // Filtrar errores del primer dígito comparando con la lectura anterior
  if (isFirstDigitError(value, this->previousValidRead) && this->rejectedReadCount < MAX_REJECTED_READS) {
    this->rejectedReadCount++;
    spcState = ReadState::IDLE;
    return;
  }

  // Si llegamos aquí, la lectura es válida o hemos superado el límite de rechazos
  this->lastRead = value;
  this->previousValidRead = value;  // Actualizar la lectura válida anterior
  this->rejectedReadCount = 0;     // Resetear el contador de rechazos
  this->lastUpdateTime = millis();

  if (this->lastRead < this->minRead) {
    this->minRead = this->lastRead;
  } else if (this->lastRead > this->maxRead) {
    this->maxRead = this->lastRead;
  }

  rpcManager.sendMeasurementData(this->lastRead, this->minRead, this->maxRead);

  spcState = ReadState::IDLE;
}