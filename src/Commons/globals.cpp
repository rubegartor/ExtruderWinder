#include "globals.h"
#include "UI/components/general.h"
#include "pins.h"


Tensioner&   tensioner   = Tensioner::getInstance();
Puller&      puller      = Puller::getInstance();
Aligner&     aligner     = Aligner::getInstance();
Spooler&     spooler     = Spooler::getInstance();
Measurement& measurement = Measurement::getInstance();
Storage&     storage     = Storage::getInstance();
Buzzer&      buzzer      = Buzzer::getInstance();

volatile bool g_newMeasurementReady = false;

float diameter = DIAMETER_DEFAULT;
bool  alertsEnabled = ALERT_ENABLED_DEFAULT;
int   weightAlertThreshold = WEIGHT_ALERT_DEFAULT;

volatile int32_t aligner_to_move = static_cast<int32_t>(STEPS_PER_CM * (DIAMETER_DEFAULT / 10.0f));

float measurementLastRead = 0.0f;
float measurementMinRead  = 0.0f;
float measurementMaxRead  = 0.0f;

std::vector<PlasticType> plasticTypes = {
  {"PLA",    1.24f},
  {"ABS",    1.02f},
  {"PETG",   1.27f},
  {"TPU",    1.20f},
  {"ASA",    1.06f},
  {"PC",     1.20f},
  {"NYLON",  1.15f},
  {"PP",     0.90f},
  {"PET",    1.38f},
  {"PC-ABS", 1.15f}
};

uint8_t selectedPlasticTypeIndex = 0;
int     spoolCalibrationCount    = 0;

void initGlobals() {
  SPI1.begin();

  pinMode(ALIGNER_CS_PIN, OUTPUT);
  pinMode(PULLER_CS_PIN,  OUTPUT);
  pinMode(SPOOLER_CS_PIN, OUTPUT);

  digitalWrite(ALIGNER_CS_PIN, HIGH);
  digitalWrite(PULLER_CS_PIN,  HIGH);
  digitalWrite(SPOOLER_CS_PIN, HIGH);

  Display.begin();
  TouchDetector.begin();

  diameter               = storage.getFloat(DIAMETER_PREF,      DIAMETER_DEFAULT);
  selectedPlasticTypeIndex = storage.getInt(PLASTIC_TYPE_PREF,  PLASTIC_TYPE_DEFAULT);
  alertsEnabled          = storage.getBool(ALERT_ENABLED_PREF,  ALERT_ENABLED_DEFAULT);
  weightAlertThreshold   = storage.getInt(WEIGHT_ALERT_PREF,    WEIGHT_ALERT_DEFAULT);

  // Valores de flash corruptos: un índice fuera de rango leía basura de plasticTypes
  // (puntero a String inválido -> HardFault al construir la UI, bucle de arranque)
  // y un diámetro 0 deja aligner_to_move = 0 (divisiones por cero, aligner parado).
  if (selectedPlasticTypeIndex >= plasticTypes.size()) selectedPlasticTypeIndex = PLASTIC_TYPE_DEFAULT;
  if (!(diameter >= DIAMETER_MIN && diameter <= DIAMETER_MAX)) diameter = DIAMETER_DEFAULT;

  aligner_to_move = static_cast<int32_t>(STEPS_PER_CM * (diameter / 10.0f));

  measurementLastRead = 0.0f;
  measurementMinRead  = 0.0f;
  measurementMaxRead  = 0.0f;
}

void updateDiameter(float value) {
  diameter = value;

  puller.updateSetPoint(diameter);
  aligner_to_move = static_cast<int32_t>(STEPS_PER_CM * (diameter / 10.0f));

  storage.setFloat(DIAMETER_PREF, diameter);

  updateChartLimits();
}

void updateAlertsEnabled(bool value) {
  alertsEnabled = value;
  storage.setBool(ALERT_ENABLED_PREF, alertsEnabled);

  if (!alertsEnabled) {
    buzzer.stop();
  }
}

void updateWeightAlertThreshold(int16_t value) {
  weightAlertThreshold = value;
  storage.setInt(WEIGHT_ALERT_PREF, weightAlertThreshold);
}


void updatePlasticType(uint8_t index) {
  selectedPlasticTypeIndex = index;
  storage.setInt(PLASTIC_TYPE_PREF, selectedPlasticTypeIndex);
}

float getExtrudedFilamentLength() {
  return (PI * PULLER_DIAM / 1000.0f) * puller.getRevolutionCount(); // metros
}

float getExtrudedFilamentWeight() {
  const PlasticType& plastic = plasticTypes[selectedPlasticTypeIndex];
  const float density = plastic.density; // g/cm³

  float radius_cm = (diameter / 2.0f) / 10.0f;
  float area_cm2  = PI * radius_cm * radius_cm;
  float length_cm = getExtrudedFilamentLength() * 100.0f;
  float volume_cm3 = area_cm2 * length_cm;
  float weight = density * volume_cm3;

  if ((alertsEnabled && weightAlertThreshold > 0) && weight >= weightAlertThreshold) {
    buzzer.beep(BEEP_MEDIUM);
  }

  return weight;
}

// Solo reinicia mín./máx. del calibre (p. ej. tras enhebrar), sin tocar longitud ni peso
void resetMinMax() {
  measurement.minRead = measurement.maxRead = measurement.lastRead;
  measurementMinRead  = measurement.lastRead;
  measurementMaxRead  = measurement.lastRead;
}

void resetMeasurements() {
  puller.resetRevolutionCount();
  buzzer.stop();

  measurement.minRead = measurement.maxRead = measurement.lastRead;
  measurementLastRead = measurement.lastRead;
  measurementMinRead  = measurement.lastRead;
  measurementMaxRead  = measurement.lastRead;
}
